// Copyright 2020 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "aistreams/gstreamer/gstreamer_video_exporter.h"

#include <tuple>

#include "absl/strings/match.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_join.h"
#include "aistreams/base/util/packet_utils.h"
#include "aistreams/gstreamer/gstreamer_raw_image_yielder.h"
#include "aistreams/gstreamer/gstreamer_runner.h"
#include "aistreams/gstreamer/gstreamer_video_writer.h"
#include "aistreams/gstreamer/type_utils.h"
#include "aistreams/port/canonical_errors.h"
#include "aistreams/port/logging.h"
#include "aistreams/port/status.h"
#include "aistreams/port/status_macros.h"
#include "aistreams/util/file_path.h"
#include "aistreams/util/random_string.h"
#include "google/cloud/storage/client.h"
#include "google/cloud/storage/parallel_upload.h"

namespace aistreams {

namespace {

constexpr int kDefaultRandomStringLength = 5;
constexpr char kDefaultWorkerName[] = "Worker";
constexpr char kGcsUploaderName[] = "GcsUploader";
constexpr char kLocalVideoSaverName[] = "LocalVideoSaver";
constexpr char kStreamServerSourceName[] = "StreamServerSource";
constexpr char kGstreamerInputSourceName[] = "GstreamerInputSource";

// --------------------------------------------------------------------
// Basic worker-channel implementation (i.e. a tiny dataflow system).
//
// Workers run business logic and communicate values to the other workers
// through Channels. A Channel is a pcqueue for one Worker to send values
// to another. It also allows either worker to know whether each other have
// completed their processing.
//
// The typical workflow is to create a set of Workers, some Channels, and use
// the free function Attach to connect them together. See
// GstreamerVideoExporter::Run() later on for an example.

// A Channel carries values of type T.
template <typename T>
class Channel {
 public:
  // Create a channel of size `size`.
  //
  // CHECK-fails for non-positive sizes.
  explicit Channel(int size);

  // Set `worker` to be the value source/destination of this channel.
  template <typename U>
  void SetSrc(const U& worker);
  template <typename U>
  void SetDst(const U& worker);

  // Returns true if the channel has a value source/destination.
  //
  // CHECK-fails if you have not set a source/destination.
  bool HasSrc() const;
  bool HasDst() const;

  // Returns true if the source/destination worker has completed.
  //
  // CHECK-fails if you have not set a source/destination.
  bool IsSrcCompleted() const;
  bool IsDstCompleted() const;

  // Returns a reference to the pcqueue.
  //
  // Use this and the ProducerConsumerQueue API to send/receive values.
  std::shared_ptr<ProducerConsumerQueue<T>> pcqueue() { return pcqueue_; }

  ~Channel() = default;
  Channel(const Channel&) = delete;
  Channel& operator=(const Channel&) = delete;

 private:
  std::shared_ptr<CompletionSignal> src_completion_signal_ = nullptr;
  std::shared_ptr<CompletionSignal> dst_completion_signal_ = nullptr;
  std::shared_ptr<ProducerConsumerQueue<T>> pcqueue_ = nullptr;
};

template <typename T>
Channel<T>::Channel(int size) {
  CHECK(size > 0);
  pcqueue_ = std::make_shared<ProducerConsumerQueue<T>>(size);
}

template <typename T>
template <typename U>
void Channel<T>::SetSrc(const U& worker) {
  src_completion_signal_ = worker.GetCompletionSignal();
}

template <typename T>
template <typename U>
void Channel<T>::SetDst(const U& worker) {
  dst_completion_signal_ = worker.GetCompletionSignal();
}

template <typename T>
bool Channel<T>::HasSrc() const {
  return src_completion_signal_ != nullptr;
}

template <typename T>
bool Channel<T>::HasDst() const {
  return dst_completion_signal_ != nullptr;
}

template <typename T>
bool Channel<T>::IsSrcCompleted() const {
  CHECK(src_completion_signal_ != nullptr);
  return src_completion_signal_->IsCompleted();
}

template <typename T>
bool Channel<T>::IsDstCompleted() const {
  CHECK(dst_completion_signal_ != nullptr);
  return dst_completion_signal_->IsCompleted();
}

// CRTP base class for Workers.
//
// A Worker consumes any number of inputs and produces any number of outputs.
// These input/output values all flow through Channels of the given type.
//
// For example, to create a new Worker that accepts int values from a Channel
// and produces double values to another Channel, do the following:
//
// class MySpecialWorker : Worker<MySpecialWorker,
//                                std::tuple<int>,
//                                std::tuple<double>> {
//    // your implementation goes here.
// };
//
// See StreamServerSource or LocalVideoSaver for more examples.
template <typename T, typename... Ts>
class Worker;

template <typename T, typename... Is, typename... Os>
class Worker<T, std::tuple<Is...>, std::tuple<Os...>> {
 public:
  // Async call to start working in the background.
  void Work();

  // Wait for the worker to complete up to a `timeout`.
  //
  // If the worker has completed, reclaim the background thread and return true.
  // Otherwise, let the worker keep working and return false.
  bool Join(absl::Duration timeout);

  // Get/Set the name of this worker.
  void SetName(const std::string& name);
  std::string GetName();

  // Get the return Status of the specific logic executed by this Worker.
  //
  // Call this after you have successfully Join()'d.
  Status GetStatus();

  ~Worker();
  Worker() = default;
  Worker(const Worker&) = delete;
  Worker& operator=(const Worker&) = delete;

  std::shared_ptr<CompletionSignal> GetCompletionSignal() const {
    return completion_signal_;
  }

  template <int N>
  std::shared_ptr<
      Channel<typename std::tuple_element<N, std::tuple<Is...>>::type>>&
  GetInputChannel() {
    return std::get<N>(in_channels_);
  }

  template <int N>
  std::shared_ptr<
      Channel<typename std::tuple_element<N, std::tuple<Os...>>::type>>&
  GetOutputChannel() {
    return std::get<N>(out_channels_);
  }

 private:
  T* derived() { return static_cast<T*>(this); }

  std::string name_ = kDefaultWorkerName;
  std::thread worker_thread_;
  std::shared_ptr<CompletionSignal> completion_signal_ =
      std::make_shared<CompletionSignal>();

 protected:
  std::tuple<std::shared_ptr<Channel<Is>>...> in_channels_;
  std::tuple<std::shared_ptr<Channel<Os>>...> out_channels_;
};

template <typename T, typename... Is, typename... Os>
void Worker<T, std::tuple<Is...>, std::tuple<Os...>>::Work() {
  completion_signal_->Start();
  worker_thread_ = std::thread([this]() {
    auto status = derived()->WorkImpl();
    completion_signal_->SetStatus(status);
    completion_signal_->End();
    return;
  });
  return;
}

template <typename T, typename... Is, typename... Os>
bool Worker<T, std::tuple<Is...>, std::tuple<Os...>>::Join(
    absl::Duration timeout) {
  if (!completion_signal_->WaitUntilCompleted(timeout)) {
    return false;
  }
  if (worker_thread_.joinable()) {
    worker_thread_.join();
  }
  return true;
}

template <typename T, typename... Is, typename... Os>
void Worker<T, std::tuple<Is...>, std::tuple<Os...>>::SetName(
    const std::string& name) {
  name_ = name;
}

template <typename T, typename... Is, typename... Os>
std::string Worker<T, std::tuple<Is...>, std::tuple<Os...>>::GetName() {
  return name_;
}

template <typename T, typename... Is, typename... Os>
Status Worker<T, std::tuple<Is...>, std::tuple<Os...>>::GetStatus() {
  return completion_signal_->GetStatus();
}

template <typename T, typename... Is, typename... Os>
Worker<T, std::tuple<Is...>, std::tuple<Os...>>::~Worker() {
  if (worker_thread_.joinable()) {
    worker_thread_.detach();
  }
}

// Attach `channel` from the `M`th output of `src_worker` to the `N`th
// input of the `dst_worker`.
template <int M, int N, typename T, typename SrcT, typename DstT>
Status Attach(std::shared_ptr<Channel<T>> channel,
              std::shared_ptr<SrcT> src_worker,
              std::shared_ptr<DstT> dst_worker) {
  if (channel == nullptr) {
    return InvalidArgumentError("Given a nullptr to the Channel.");
  }
  if (src_worker == nullptr) {
    return InvalidArgumentError("Given a nullptr to the source Worker.");
  }
  if (dst_worker == nullptr) {
    return InvalidArgumentError("Given a nullptr to the destination Worker.");
  }
  channel->SetSrc(*src_worker);
  channel->SetDst(*dst_worker);
  src_worker->template GetOutputChannel<M>() = channel;
  dst_worker->template GetInputChannel<N>() = channel;
  return OkStatus();
}

// --------------------------------------------------------------------
// GstreamerInputSource implementation.
//
// TODO: Can the StreamServerSource not just be GstreamerInputSource with an
// aissrc in the beginning? Probably yes, but more features will be needed to
// control the gstreamer pipeline (most likely by adding features to
// GstreamerRunner).

class GstreamerInputSource : public Worker<GstreamerInputSource, std::tuple<>,
                                           std::tuple<StatusOr<RawImage>>> {
 public:
  struct Options {
    std::string gstreamer_input_pipeline;
  };
  explicit GstreamerInputSource(const Options& options) : options_(options) {}

  Status WorkImpl() {
    AIS_RETURN_IF_ERROR(ValidatePreconditions());

    // Create and run the gstreamer input pipeline.
    GstreamerRunner::Options gstreamer_runner_options;
    gstreamer_runner_options.appsink_sync = true;
    gstreamer_runner_options.processing_pipeline_string =
        DecideProcessingPipeline();
    gstreamer_runner_options.receiver_callback =
        [this](GstreamerBuffer buffer) -> Status {
      auto raw_image_statusor = ToRawImage(std::move(buffer));
      if (!raw_image_statusor.ok()) {
        return raw_image_statusor.status();
      }
      if (out_channel()->IsDstCompleted()) {
        return CancelledError(absl::StrFormat(
            "%s: The downstream worker has completed.", GetName()));
      }
      if (!out_channel()->pcqueue()->TryEmplace(
              std::move(raw_image_statusor))) {
        LOG(ERROR)
            << "The working raw image buffer is full; dropping frame. Consider "
               "increasing the working buffer size if you believe this is "
               "transient. Otherwise, your input source's frame rate may be "
               "too high; please contact us to let us know your use case.";
      }
      return OkStatus();
    };
    auto gstreamer_runner_statusor =
        GstreamerRunner::Create(gstreamer_runner_options);
    if (!gstreamer_runner_statusor.ok()) {
      return gstreamer_runner_statusor.status();
    }
    auto gstreamer_runner = std::move(gstreamer_runner_statusor).ValueOrDie();

    // Wait for the runner to complete.
    //
    // TODO: Add support to get GstreamerRunner's status on completion.
    while (!gstreamer_runner->WaitUntilCompleted(absl::Seconds(5)))
      ;

    // Cleanup.
    if (!out_channel()->pcqueue()->TryEmplace(NotFoundError("Reached EOS."))) {
      LOG(WARNING) << absl::StrFormat(
          "%s: Failed to deliver EOS to dependent workers.", GetName());
    }
    return OkStatus();
  }

 private:
  Options options_;

  std::shared_ptr<Channel<StatusOr<RawImage>>> out_channel() {
    return std::get<0>(out_channels_);
  }

  Status ValidatePreconditions() {
    if (options_.gstreamer_input_pipeline.empty()) {
      return InvalidArgumentError(absl::StrFormat(
          "%s: You must specify a non-empty gstreamer input pipeline.",
          GetName()));
    }
    if (out_channel() == nullptr) {
      return FailedPreconditionError(absl::StrFormat(
          "%s: No output channel found; please Attach() one.", GetName()));
    }
    if (!out_channel()->HasDst()) {
      return InternalError(absl::StrFormat(
          "%s: The output channel has no destination.", GetName()));
    }
    return OkStatus();
  }

  std::string DecideProcessingPipeline() {
    return absl::StrFormat("%s ! videoconvert ! video/x-raw,format=RGB",
                           options_.gstreamer_input_pipeline);
  }
};

// --------------------------------------------------------------------
// StreamServerSource implementation.

class StreamServerSource : public Worker<StreamServerSource, std::tuple<>,
                                         std::tuple<StatusOr<RawImage>>> {
 public:
  struct Options {
    ReceiverOptions receiver_options;
    absl::Duration receiver_timeout = absl::Seconds(10);
  };
  explicit StreamServerSource(const Options& options) : options_(options) {}

  Status WorkImpl() {
    AIS_RETURN_IF_ERROR(ValidatePreconditions());

    // TODO: May need to be in a retry loop to re-establish connections in the
    // future for the poor connectivity use cases.
    AIS_RETURN_IF_ERROR(ConnectAndWarmup());

    // Main fetch/decode loop.
    Status return_status = OkStatus();
    while (!out_channel()->IsDstCompleted()) {
      // Get a packet and check for EOS.
      bool is_eos = false;
      StatusOr<Packet> packet_statusor = ReceivePacket(&is_eos);
      if (!packet_statusor.ok()) {
        return_status = UnknownError(
            absl::StrFormat("Failed to receive a packet from upstream: %s",
                            packet_statusor.status().message()));
        break;
      }
      if (is_eos) {
        break;
      }

      // Feed the Packet for decoding.
      auto gstreamer_buffer_statusor =
          ToGstreamerBuffer(std::move(packet_statusor).ValueOrDie());
      if (!gstreamer_buffer_statusor.ok()) {
        return_status = UnknownError(absl::StrFormat(
            "Failed to convert a packet to a gstreamer buffer: %s",
            gstreamer_buffer_statusor.status().message()));
        break;
      }
      auto status = raw_image_yielder_->Feed(
          std::move(gstreamer_buffer_statusor).ValueOrDie());
      if (!status.ok()) {
        return_status = UnknownError(absl::StrFormat(
            "Failed to Feed the data for raw image conversion: %s",
            status.message()));
        break;
      }
    }

    raw_image_yielder_->SignalEOS();
    return return_status;
  }

  ~StreamServerSource() = default;

 private:
  StatusOr<Packet> ReceivePacket(bool* is_eos) {
    Packet p;
    if (!packet_receiver_queue_->TryPop(p, options_.receiver_timeout)) {
      return DeadlineExceededError(absl::StrFormat(
          "Failed to receive a packet within the specified timeout (%s).",
          FormatDuration(options_.receiver_timeout)));
    }
    *is_eos = IsEos(p);
    return p;
  }

  Status ValidatePreconditions() {
    if (out_channel() == nullptr) {
      return FailedPreconditionError(absl::StrFormat(
          "%s: No output channel found; please Attach() one.", GetName()));
    }
    if (!out_channel()->HasDst()) {
      return InternalError(absl::StrFormat(
          "%s: The output channel has no destination.", GetName()));
    }
    return OkStatus();
  }

  Status ConnectAndWarmup() {
    // Create the packet reciever queue.
    packet_receiver_queue_ = std::make_unique<ReceiverQueue<Packet>>();
    auto status = MakePacketReceiverQueue(options_.receiver_options,
                                          packet_receiver_queue_.get());
    if (!status.ok()) {
      return InvalidArgumentError(absl::StrFormat(
          "Failed to create a packet receiver queue: %s", status.message()));
    }

    // Receive the first packet and convert it to a gstreamer buffer.
    bool is_eos = false;
    StatusOr<Packet> packet_statusor = ReceivePacket(&is_eos);
    if (!packet_statusor.ok()) {
      return UnknownError(
          absl::StrFormat("Failed to receive a packet from upstream: %s",
                          packet_statusor.status().message()));
    }
    if (is_eos) {
      return NotFoundError("Got EOS. The stream has already ended.");
    }
    auto gstreamer_buffer_statusor =
        ToGstreamerBuffer(std::move(packet_statusor).ValueOrDie());
    if (!gstreamer_buffer_statusor.ok()) {
      return UnknownError(absl::StrFormat(
          "Failed to convert the first packet to a gstreamer buffer: %s",
          gstreamer_buffer_statusor.status().message()));
    }
    auto gstreamer_buffer = std::move(gstreamer_buffer_statusor).ValueOrDie();

    // Create the raw image yielder.
    GstreamerRawImageYielder::Options raw_image_yielder_options;
    raw_image_yielder_options.caps_string = gstreamer_buffer.get_caps();
    raw_image_yielder_options.callback =
        [this](StatusOr<RawImage> raw_image_statusor) -> Status {
      if (!out_channel()->pcqueue()->TryEmplace(
              std::move(raw_image_statusor))) {
        LOG(ERROR)
            << "The working raw image buffer is full; dropping frame. Consider "
               "increasing the working buffer size if you believe this is "
               "transient. Otherwise, your input source's frame rate may be "
               "too high; please contact us to let us know your use case.";
      }
      return OkStatus();
    };
    auto raw_image_yielder_statusor =
        GstreamerRawImageYielder::Create(raw_image_yielder_options);
    if (!raw_image_yielder_statusor.ok()) {
      return raw_image_yielder_statusor.status();
    }
    raw_image_yielder_ = std::move(raw_image_yielder_statusor).ValueOrDie();

    // Feed the first buffer.
    status = raw_image_yielder_->Feed(gstreamer_buffer);
    if (!status.ok()) {
      return UnknownError(absl::StrFormat(
          "Failed to Feed the first buffer into the raw image yielder: %s",
          status.message()));
    }
    return OkStatus();
  }

  Options options_;

  std::shared_ptr<Channel<StatusOr<RawImage>>> out_channel() {
    return std::get<0>(out_channels_);
  }

  std::unique_ptr<ReceiverQueue<Packet>> packet_receiver_queue_ = nullptr;

  // TODO: Consider pushing the decoder into the local video saver.
  std::unique_ptr<GstreamerRawImageYielder> raw_image_yielder_ = nullptr;
};

// --------------------------------------------------------------------
// LocalVideoSaver implementation.

class LocalVideoSaver
    : public Worker<LocalVideoSaver, std::tuple<StatusOr<RawImage>>,
                    std::tuple<StatusOr<std::string>>> {
 public:
  struct Options {
    std::string file_prefix;
    std::string output_dir;
    int max_frames_per_file;

    // TODO: Consider refactoring to have two Workers, one having no output
    // stream and another having one so that we can avoid this conditional
    // behavior that muddies the interface semantics.
    bool forward_file_paths;
  };
  explicit LocalVideoSaver(const Options& options) : options_(options) {}

  Status WorkImpl() {
    AIS_RETURN_IF_ERROR(ValidatePreconditions());

    Status return_status = OkStatus();
    for (bool start_new_file = true; start_new_file;) {
      std::unique_ptr<GstreamerVideoWriter> video_writer = nullptr;
      std::string output_video_file_path;
      for (int image_index = 0; image_index < options_.max_frames_per_file;
           ++image_index) {
        // Get a new raw image from the decoder.
        StatusOr<RawImage> raw_image_statusor;
        while (!in_channel()->pcqueue()->TryPop(raw_image_statusor,
                                                absl::Seconds(5))) {
          if (in_channel()->IsSrcCompleted()) {
            raw_image_statusor = NotFoundError(
                "The image source completed without delivering an EOS.");
            break;
          }
        }
        if (raw_image_statusor.status().code() == StatusCode::kNotFound) {
          start_new_file = false;
          break;
        }

        // Get the raw image and write it into the current video file.
        auto raw_image_gstreamer_buffer_statusor =
            ToGstreamerBuffer(std::move(raw_image_statusor).ValueOrDie());
        if (!raw_image_gstreamer_buffer_statusor.ok()) {
          return_status = UnknownError(absl::StrFormat(
              "Could not convert a raw image into a gstreamer buffer: %s",
              raw_image_gstreamer_buffer_statusor.status().message()));
          break;
        }
        auto raw_image_gstreamer_buffer =
            std::move(raw_image_gstreamer_buffer_statusor).ValueOrDie();

        if (video_writer == nullptr) {
          output_video_file_path = GenerateVideoFilePath();
          GstreamerVideoWriter::Options video_writer_options;
          video_writer_options.file_path = output_video_file_path;
          video_writer_options.caps_string =
              raw_image_gstreamer_buffer.get_caps();
          auto video_writer_statusor =
              GstreamerVideoWriter::Create(video_writer_options);
          if (!video_writer_statusor.ok()) {
            return_status = InternalError(
                absl::StrFormat("Failed to create a new video writer: %s",
                                video_writer_statusor.status().message()));
            break;
          }
          video_writer = std::move(video_writer_statusor).ValueOrDie();
        }

        auto status = video_writer->Put(raw_image_gstreamer_buffer);
        if (!status.ok()) {
          return_status = UnknownError(absl::StrFormat(
              "Failed to write a raw image: %s", status.message()));
          break;
        }
      }

      // Explicitly flush the video file.
      video_writer.reset(nullptr);
      if (!return_status.ok()) {
        if (std::remove(output_video_file_path.c_str()) != 0) {
          LOG(WARNING) << absl::StrFormat("%s: Failed to remove %s.", GetName(),
                                          output_video_file_path);
        }
        start_new_file = false;
        continue;
      }
      LOG(INFO) << absl::StrFormat("%s: Successfully wrote local file %s.",
                                   GetName(), output_video_file_path);

      if (options_.forward_file_paths) {
        // Forward the video path if downstream is still running.
        // Stop processing otherwise.
        if (!out_channel()->IsDstCompleted()) {
          if (!out_channel()->pcqueue()->TryEmplace(output_video_file_path)) {
            LOG(WARNING) << absl::StrFormat(
                "%s: The file path buffer is full. Omitting %s from "
                "downstream processing.",
                GetName(), output_video_file_path);
          }
        } else {
          start_new_file = false;
          continue;
        }
      }
    }

    // Cleanup.
    if (options_.forward_file_paths) {
      if (!out_channel()->pcqueue()->TryEmplace(
              NotFoundError("Reached EOS."))) {
        LOG(WARNING) << absl::StrFormat(
            "%s: Failed to deliver EOS to dependent workers.", GetName());
      }
    }
    return return_status;
  }

 private:
  Options options_;

  std::shared_ptr<Channel<StatusOr<RawImage>>> in_channel() {
    return std::get<0>(in_channels_);
  }

  std::shared_ptr<Channel<StatusOr<std::string>>> out_channel() {
    return std::get<0>(out_channels_);
  }

  Status ValidatePreconditions() {
    if (options_.max_frames_per_file <= 0) {
      return InvalidArgumentError(
          absl::StrFormat("%s: A positive value for the maximum frame count is "
                          "expected (given %d)",
                          GetName(), options_.max_frames_per_file));
    }
    if (in_channel() == nullptr) {
      return FailedPreconditionError(absl::StrFormat(
          "%s: No input channel found; please Attach() one.", GetName()));
    }
    if (!in_channel()->HasSrc()) {
      return InternalError(
          absl::StrFormat("%s: The input channel has no source.", GetName()));
    }

    if (options_.forward_file_paths) {
      if (out_channel() == nullptr) {
        return FailedPreconditionError(absl::StrFormat(
            "%s: No output channel found; please Attach() one.", GetName()));
      }
      if (!out_channel()->HasDst()) {
        return InternalError(absl::StrFormat(
            "%s: The output channel has no destination.", GetName()));
      }
    }
    return OkStatus();
  }

  std::string GenerateVideoFilePath() {
    // TODO: Name the files according to server conventions.
    //
    // Currently, we use the following:
    // [<optional-prefix>-]<random-session-id>-<time-string>.mp4
    //
    // The <time-string> is in absl's default human readable format (RFC3339).
    // and uses the local time.
    //
    // It is better to use the packet timestamp for the stream server
    // source and the gstreamer buffer time for the gstreamer input source
    // (although one should investigate whether the buffer's timestamp
    // corresponds to the event's local time; for on-device sources, this is
    // close to absl::Now(), but not necessarily for network sources).
    static const std::string session_string =
        RandomString(kDefaultRandomStringLength);
    std::string time_string = absl::FormatTime(absl::Now());
    std::vector<std::string> file_name_components;
    if (!options_.file_prefix.empty()) {
      file_name_components.push_back(options_.file_prefix);
    }
    file_name_components.push_back(session_string);
    file_name_components.push_back(time_string);
    std::string file_name = absl::StrJoin(file_name_components, "-");
    file_name += ".mp4";

    // Decide the output file path.
    std::string file_path = file_name;
    if (!options_.output_dir.empty()) {
      file_path = absl::StrFormat("%s/%s", options_.output_dir, file_name);
    }
    return file_path;
  }
};

// --------------------------------------------------------------------
// GcsUploader implementation.

class GcsUploader
    : public Worker<GcsUploader, std::tuple<StatusOr<std::string>>,
                    std::tuple<>> {
 public:
  struct Options {
    bool do_work;
    std::string gcs_bucket_name;
    std::string gcs_object_dir;
    bool keep_local;
  };
  explicit GcsUploader(const Options& options) : options_(options) {}

  Status WorkImpl() {
    if (!options_.do_work) {
      return OkStatus();
    }
    AIS_RETURN_IF_ERROR(ValidatePreconditions());

    for (bool start_new_connection = true; start_new_connection;) {
      google::cloud::StatusOr<google::cloud::storage::Client> client_statusor =
          google::cloud::storage::Client::CreateDefaultClient();
      if (!client_statusor.ok()) {
        return UnknownError(
            absl::StrFormat("%s: Failed to create a GCS Client: %s", GetName(),
                            client_statusor.status().message()));
      }
      auto gcs_client = std::move(client_statusor).value();

      while (true) {
        // Get the path to a local video file.
        StatusOr<std::string> file_path_statusor;
        while (!in_channel()->pcqueue()->TryPop(file_path_statusor,
                                                absl::Seconds(5))) {
          if (in_channel()->IsSrcCompleted()) {
            file_path_statusor = NotFoundError(
                absl::StrFormat("%s: The file path source completed but the "
                                "EOS was not delivered.",
                                GetName()));
            break;
          }
        }
        if (file_path_statusor.status().code() == StatusCode::kNotFound) {
          start_new_connection = false;
          break;
        }

        // Upload the file.
        auto file_path = std::move(file_path_statusor).ValueOrDie();
        auto object_name = GenerateGcsObjectName(file_path);
        google::cloud::StatusOr<google::cloud::storage::ObjectMetadata>
            metadata = gcs_client.UploadFile(
                file_path, options_.gcs_bucket_name, object_name);

        // TODO: Currently we force all errors to recover by trying to get a new
        // GCS connection. Clearly, this can be improved, but it works as a big
        // hammer to take care of poor connectivity cases.
        if (!metadata.ok()) {
          LOG(WARNING) << absl::StrFormat("%s: Failed to upload %s to GCS: %s",
                                          GetName(), file_path,
                                          metadata.status().message());
          break;
        }
        LOG(INFO) << absl::StrFormat(
            "%s: Successfully uploaded %s to gs://%s/%s.", GetName(), file_path,
            (*metadata).bucket(), (*metadata).name());

        // Remove the local file.
        if (!options_.keep_local) {
          if (std::remove(file_path.c_str()) != 0) {
            LOG(WARNING) << absl::StrFormat("%s: Failed to remove %s.",
                                            GetName(), file_path);
          }
        }
      }
    }
    return OkStatus();
  }

 private:
  Options options_;

  Status ValidatePreconditions() {
    if (options_.gcs_bucket_name.empty()) {
      return InvalidArgumentError(absl::StrFormat(
          "%s: You must supply a non-empty GCS bucket name.", GetName()));
    }
    if (in_channel() == nullptr) {
      return FailedPreconditionError(absl::StrFormat(
          "%s: No input channel found; please Attach() one.", GetName()));
    }
    if (!in_channel()->HasSrc()) {
      return InternalError(
          absl::StrFormat("%s: The input channel has no source.", GetName()));
    }
    return OkStatus();
  }

  std::shared_ptr<Channel<StatusOr<std::string>>> in_channel() {
    return std::get<0>(in_channels_);
  }

  std::string GenerateGcsObjectName(const std::string& file_path) {
    std::string file_name(file::Basename(file_path));
    if (options_.gcs_object_dir.empty()) {
      return file_name;
    }
    if (!absl::EndsWith(options_.gcs_object_dir, "/")) {
      return absl::StrFormat("%s/%s", options_.gcs_object_dir, file_name);
    } else {
      return absl::StrFormat("%s%s", options_.gcs_object_dir, file_name);
    }
  }
};

}  // namespace

// --------------------------------------------------------------------
// GstreamerVideoExporter implementation.

GstreamerVideoExporter::GstreamerVideoExporter(const Options& options)
    : options_(options) {}

StatusOr<std::unique_ptr<GstreamerVideoExporter>>
GstreamerVideoExporter::Create(const Options& options) {
  if (options.working_buffer_size <= 0) {
    return InvalidArgumentError(
        "You must supply a positive value for the working buffer size");
  }
  auto video_exporter = std::make_unique<GstreamerVideoExporter>(options);
  return video_exporter;
}

Status GstreamerVideoExporter::Run() {
  if (has_been_run_) {
    return FailedPreconditionError(
        "This video exporter has already been Run. Please Create a new "
        "instance and try again.");
  }
  has_been_run_ = true;

  // Create the workers and connect them up with channels.
  GcsUploader::Options gcs_uploader_options;
  gcs_uploader_options.do_work = options_.upload_to_gcs;
  gcs_uploader_options.gcs_bucket_name = options_.gcs_bucket_name;
  gcs_uploader_options.gcs_object_dir = options_.gcs_object_dir;
  gcs_uploader_options.keep_local = options_.keep_local;
  auto gcs_uploader = std::make_shared<GcsUploader>(gcs_uploader_options);
  gcs_uploader->SetName(kGcsUploaderName);

  LocalVideoSaver::Options local_video_saver_options;
  local_video_saver_options.output_dir = options_.output_dir;
  local_video_saver_options.file_prefix = options_.file_prefix;
  local_video_saver_options.max_frames_per_file = options_.max_frames_per_file;
  local_video_saver_options.forward_file_paths = options_.upload_to_gcs;
  auto local_video_saver =
      std::make_shared<LocalVideoSaver>(local_video_saver_options);
  local_video_saver->SetName(kLocalVideoSaverName);

  StreamServerSource::Options stream_server_source_options;
  stream_server_source_options.receiver_options = options_.receiver_options;
  auto stream_server_source =
      std::make_shared<StreamServerSource>(stream_server_source_options);
  stream_server_source->SetName(kStreamServerSourceName);

  GstreamerInputSource::Options gstreamer_input_source_options;
  gstreamer_input_source_options.gstreamer_input_pipeline =
      options_.gstreamer_input_pipeline;
  auto gstreamer_input_source =
      std::make_shared<GstreamerInputSource>(gstreamer_input_source_options);
  gstreamer_input_source->SetName(kGstreamerInputSourceName);

  auto file_path_channel = std::make_shared<Channel<StatusOr<std::string>>>(
      options_.working_buffer_size);
  auto status =
      Attach<0, 0>(file_path_channel, local_video_saver, gcs_uploader);
  if (!status.ok()) {
    LOG(ERROR) << status;
    return UnknownError(
        absl::StrFormat("Failed to Attach workers \"%s\" and \"%s\".",
                        local_video_saver->GetName(), gcs_uploader->GetName()));
  }

  auto raw_image_channel = std::make_shared<Channel<StatusOr<RawImage>>>(
      options_.working_buffer_size);
  if (options_.use_gstreamer_input_source) {
    status = Attach<0, 0>(raw_image_channel, gstreamer_input_source,
                          local_video_saver);
    if (!status.ok()) {
      LOG(ERROR) << status;
      return UnknownError(absl::StrFormat(
          "Failed to Attach workers \"%s\" and \"%s\".",
          gstreamer_input_source->GetName(), local_video_saver->GetName()));
    }
  } else {
    status = Attach<0, 0>(raw_image_channel, stream_server_source,
                          local_video_saver);
    if (!status.ok()) {
      LOG(ERROR) << status;
      return UnknownError(absl::StrFormat(
          "Failed to Attach workers \"%s\" and \"%s\".",
          stream_server_source->GetName(), local_video_saver->GetName()));
    }
  }

  // Start the workers.
  gcs_uploader->Work();
  local_video_saver->Work();

  // Wait for the video source to complete.
  Status video_source_status = OkStatus();
  if (options_.use_gstreamer_input_source) {
    gstreamer_input_source->Work();
    while (!gstreamer_input_source->Join(absl::Seconds(5)))
      ;
    video_source_status = gstreamer_input_source->GetStatus();
  } else {
    stream_server_source->Work();
    while (!stream_server_source->Join(absl::Seconds(5)))
      ;
    video_source_status = stream_server_source->GetStatus();
  }

  // Join the other workers.
  std::string deadline_exceeded_message =
      "\"%s\" did not finalize its work in time. It will be detached.";
  Status local_video_saver_status = OkStatus();
  if (!local_video_saver->Join(options_.finalization_deadline)) {
    local_video_saver_status = DeadlineExceededError(
        absl::StrFormat(absl::string_view(deadline_exceeded_message),
                        local_video_saver->GetName()));
  } else {
    local_video_saver_status = local_video_saver->GetStatus();
  }
  Status gcs_uploader_status = OkStatus();
  if (!gcs_uploader->Join(options_.finalization_deadline)) {
    gcs_uploader_status = DeadlineExceededError(absl::StrFormat(
        absl::string_view(deadline_exceeded_message), gcs_uploader->GetName()));
  } else {
    gcs_uploader_status = gcs_uploader->GetStatus();
  }

  // Report errors.
  Status return_status = OkStatus();
  Status error_status =
      UnknownError("The Run() did not complete successfully.");
  if (!video_source_status.ok()) {
    LOG(ERROR) << video_source_status;
    return_status = error_status;
  }
  if (!local_video_saver_status.ok()) {
    LOG(ERROR) << local_video_saver_status;
    return_status = error_status;
  }
  if (!gcs_uploader_status.ok()) {
    LOG(ERROR) << gcs_uploader_status;
    return_status = error_status;
  }
  return return_status;
}

GstreamerVideoExporter::~GstreamerVideoExporter() {}

}  // namespace aistreams
