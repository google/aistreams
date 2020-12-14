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

#include "absl/strings/str_format.h"
#include "absl/strings/str_join.h"
#include "aistreams/base/util/packet_utils.h"
#include "aistreams/gstreamer/gstreamer_video_writer.h"
#include "aistreams/gstreamer/type_utils.h"
#include "aistreams/port/canonical_errors.h"
#include "aistreams/port/logging.h"
#include "aistreams/port/status.h"
#include "aistreams/port/status_macros.h"
#include "aistreams/util/random_string.h"

namespace aistreams {

namespace {

constexpr char kDefaultWorkerName[] = "Worker";
constexpr int kDefaultRandomStringLength = 5;
constexpr char kLocalVideoSaverName[] = "LocalVideoSaver";
constexpr char kStreamServerSourceName[] = "StreamServerSource";

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
                    std::tuple<>> {
 public:
  struct Options {
    std::string file_prefix;
    int max_frames_per_file;
  };
  explicit LocalVideoSaver(const Options& options) : options_(options) {}

  Status WorkImpl() {
    AIS_RETURN_IF_ERROR(ValidatePreconditions());

    Status return_status = OkStatus();
    for (bool start_new_file = true; start_new_file;) {
      std::unique_ptr<GstreamerVideoWriter> video_writer = nullptr;
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
          start_new_file = false;
          break;
        }
        auto raw_image_gstreamer_buffer =
            std::move(raw_image_gstreamer_buffer_statusor).ValueOrDie();

        if (video_writer == nullptr) {
          auto video_writer_statusor =
              CreateVideoWriter(raw_image_gstreamer_buffer.get_caps());
          if (!video_writer_statusor.ok()) {
            return_status = InternalError(
                absl::StrFormat("Failed to create a new video writer: %s",
                                video_writer_statusor.status().message()));
            start_new_file = false;
            break;
          }
          video_writer = std::move(video_writer_statusor).ValueOrDie();
        }

        auto status = video_writer->Put(raw_image_gstreamer_buffer);
        if (!status.ok()) {
          return_status = UnknownError(absl::StrFormat(
              "Failed to write a raw image: %s", status.message()));
          start_new_file = false;
          break;
        }
      }
    }
    return return_status;
  }

 private:
  Options options_;

  std::shared_ptr<Channel<StatusOr<RawImage>>> in_channel() {
    return std::get<0>(in_channels_);
  }

  Status ValidatePreconditions() {
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

  StatusOr<std::unique_ptr<GstreamerVideoWriter>> CreateVideoWriter(
      const std::string& caps_string) {
    // TODO: Name the files according to server conventions.
    //
    // Currently, we use the following:
    // [<optional-prefix>-]<random-session-id>-<time-string>.mp4
    //
    // The <time-string> is in absl's default human readable format (RFC3339).
    static const std::string session_string =
        RandomString(kDefaultRandomStringLength);
    std::string time_string = absl::FormatTime(absl::Now());
    std::vector<std::string> file_name_components;
    if (!options_.file_prefix.empty()) {
      file_name_components.push_back(options_.file_prefix);
    }
    file_name_components.push_back(session_string);
    file_name_components.push_back(time_string);
    std::string filename = absl::StrJoin(file_name_components, "-");
    filename += ".mp4";

    GstreamerVideoWriter::Options video_writer_options;
    video_writer_options.filename = filename;
    video_writer_options.caps_string = caps_string;
    return GstreamerVideoWriter::Create(video_writer_options);
  }
};

// --------------------------------------------------------------------
// GstreamerVideoExporter implementation.

Status ValidateOptions(const GstreamerVideoExporter::Options& options) {
  if (options.max_frames_per_file <= 0) {
    return InvalidArgumentError(
        "You must supply a positive value for the maximum frame count of each "
        "video");
  }

  if (options.working_buffer_size <= 0) {
    return InvalidArgumentError(
        "You must supply a positive value for the working buffer size");
  }

  return OkStatus();
}

}  // namespace

GstreamerVideoExporter::GstreamerVideoExporter(const Options& options)
    : options_(options) {}

StatusOr<std::unique_ptr<GstreamerVideoExporter>>
GstreamerVideoExporter::Create(const Options& options) {
  auto status = ValidateOptions(options);
  if (!status.ok()) {
    return status;
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
  LocalVideoSaver::Options local_video_saver_options;
  local_video_saver_options.file_prefix = options_.file_prefix;
  local_video_saver_options.max_frames_per_file = options_.max_frames_per_file;
  auto local_video_saver =
      std::make_shared<LocalVideoSaver>(local_video_saver_options);
  local_video_saver->SetName(kLocalVideoSaverName);

  StreamServerSource::Options stream_server_source_options;
  stream_server_source_options.receiver_options = options_.receiver_options;
  auto stream_server_source =
      std::make_shared<StreamServerSource>(stream_server_source_options);
  stream_server_source->SetName(kStreamServerSourceName);

  auto raw_image_channel = std::make_shared<Channel<StatusOr<RawImage>>>(
      options_.working_buffer_size);
  auto status =
      Attach<0, 0>(raw_image_channel, stream_server_source, local_video_saver);
  if (!status.ok()) {
    LOG(ERROR) << status;
    return UnknownError(absl::StrFormat(
        "Failed to Attach workers \"%s\" and \"%d\".",
        stream_server_source->GetName(), local_video_saver->GetName()));
  }

  // Start the workers.
  local_video_saver->Work();
  stream_server_source->Work();

  // Wait for the stream source to complete.
  while (!stream_server_source->Join(absl::Seconds(5)))
    ;
  Status stream_server_source_status = stream_server_source->GetStatus();

  // Join the other workers.
  std::string deadline_exceeded_message =
      "\"%s\" did not finalize its work in time. It will be detached.";
  Status writer_status = OkStatus();
  if (!local_video_saver->Join(options_.finalization_deadline)) {
    writer_status = DeadlineExceededError(
        absl::StrFormat(absl::string_view(deadline_exceeded_message),
                        local_video_saver->GetName()));
  } else {
    writer_status = local_video_saver->GetStatus();
  }

  // Report errors.
  Status return_status = OkStatus();
  Status error_status =
      UnknownError("The Run() did not complete successfully.");
  if (!stream_server_source_status.ok()) {
    LOG(ERROR) << stream_server_source_status;
    return_status = error_status;
  }
  if (!writer_status.ok()) {
    LOG(ERROR) << writer_status;
    return_status = error_status;
  }
  return return_status;
}

GstreamerVideoExporter::~GstreamerVideoExporter() {}

}  // namespace aistreams
