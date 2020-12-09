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

#include "absl/strings/str_format.h"
#include "absl/strings/str_join.h"
#include "aistreams/base/util/packet_utils.h"
#include "aistreams/gstreamer/gstreamer_video_writer.h"
#include "aistreams/gstreamer/type_utils.h"
#include "aistreams/port/canonical_errors.h"
#include "aistreams/port/logging.h"
#include "aistreams/port/status.h"
#include "aistreams/port/status_macros.h"

namespace aistreams {

namespace {

constexpr char kDefaultFilePrefix[] = "video";

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

// Signaling object used to communicate with the writer thread.
// TODO: Refactor and combine with CompletionSignal (see gstreamer_runner.cc).
class GstreamerVideoExporter::WriterSignal {
 public:
  WriterSignal() = default;
  ~WriterSignal() = default;

  void Start() {
    absl::MutexLock lock(&mu_);
    is_completed_ = false;
  }

  void End() {
    absl::MutexLock lock(&mu_);
    is_completed_ = true;
  }

  bool IsCompleted() const {
    absl::MutexLock lock(&mu_);
    return is_completed_;
  }

  bool WaitUntilCompleted(absl::Duration timeout) {
    absl::MutexLock lock(&mu_);
    absl::Condition cond(
        +[](bool* is_completed) -> bool { return *is_completed; },
        &is_completed_);
    return mu_.AwaitWithTimeout(cond, timeout);
  }

  Status GetStatus() const {
    absl::MutexLock lock(&mu_);
    return status_;
  }

  void SetStatus(const Status& status) {
    absl::MutexLock lock(&mu_);
    status_ = status;
    return;
  }

 private:
  mutable absl::Mutex mu_;
  bool is_completed_ ABSL_GUARDED_BY(mu_) = true;
  Status status_ ABSL_GUARDED_BY(mu_) = OkStatus();
};

// --------------------------------------------------------------------
// GstreamerVideoExporter implementation.

GstreamerVideoExporter::GstreamerVideoExporter(const Options& options)
    : options_(options) {}

StatusOr<std::unique_ptr<GstreamerVideoExporter>>
GstreamerVideoExporter::Create(const Options& options) {
  // Construct the exporter and initialize the configuration data.
  auto status = ValidateOptions(options);
  if (!status.ok()) {
    return status;
  }
  auto video_exporter = std::make_unique<GstreamerVideoExporter>(options);
  if (video_exporter->options_.file_prefix.empty()) {
    video_exporter->options_.file_prefix = kDefaultFilePrefix;
  }

  // Initialize the exporter.
  status = video_exporter->Initialize();
  if (!status.ok()) {
    LOG(ERROR) << status;
    return InternalError("Failed to Initialize the GstreamerVideoExporter.");
  }
  return video_exporter;
}

Status GstreamerVideoExporter::Initialize() {
  // Create the packet reciever queue.
  packet_receiver_queue_ = std::make_unique<ReceiverQueue<Packet>>();
  auto status = MakePacketReceiverQueue(options_.receiver_options,
                                        packet_receiver_queue_.get());
  if (!status.ok()) {
    LOG(ERROR) << status;
    return InvalidArgumentError("Failed to create a packet receiver queue.");
  }

  // Create the raw image pcqueue between the receiver and writer threads.
  raw_image_pcqueue_ =
      std::make_shared<ProducerConsumerQueue<StatusOr<RawImage>>>(
          options_.working_buffer_size);

  // Get the first data element from the source and convert it into a
  // gstreamer buffer.
  auto first_gstreamer_buffer_statusor = ReceiveFirstBuffer();
  if (!first_gstreamer_buffer_statusor.ok()) {
    LOG(ERROR) << first_gstreamer_buffer_statusor.status();
    return InvalidArgumentError(
        "The given buffer source has elements that are not video compatible.");
  }
  auto first_gstreamer_buffer =
      std::move(first_gstreamer_buffer_statusor).ValueOrDie();

  // Create and warm up the raw image yielder.
  status = CreateAndWarmupRawImageYielder(std::move(first_gstreamer_buffer));
  if (!status.ok()) {
    LOG(ERROR) << status;
    return InvalidArgumentError("Failed to initialize the RawImageYielder.");
  }
  return OkStatus();
}

StatusOr<GstreamerBuffer> GstreamerVideoExporter::ReceiveFirstBuffer() {
  Packet p;
  if (!packet_receiver_queue_->TryPop(p, options_.receiver_timeout)) {
    return DeadlineExceededError(absl::StrFormat(
        "Failed to receive the first packet within the specified timeout (%s).",
        FormatDuration(options_.receiver_timeout)));
  }
  return ToGstreamerBuffer(std::move(p));
}

Status GstreamerVideoExporter::CreateAndWarmupRawImageYielder(
    GstreamerBuffer gstreamer_buffer) {
  // Create the raw image yielder.
  GstreamerRawImageYielder::Options raw_image_yielder_options;
  raw_image_yielder_options.caps_string = gstreamer_buffer.get_caps();
  raw_image_yielder_options.callback =
      [this](StatusOr<RawImage> raw_image_statusor) -> Status {
    if (!raw_image_pcqueue_->TryEmplace(std::move(raw_image_statusor))) {
      LOG(ERROR)
          << "The working raw image buffer is full; dropping frame. Consider "
             "increasing the working buffer size if you believe this is "
             "transient. Otherwise, your input source's frame rate may be too "
             "high; please contact us to let us know your use case.";
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
  auto status = raw_image_yielder_->Feed(gstreamer_buffer);
  if (!status.ok()) {
    LOG(ERROR) << status;
    return UnknownError(
        "Failed to Feed the first buffer into the raw image yielder.");
  }

  return OkStatus();
}

Status GstreamerVideoExporter::Run() {
  if (has_been_run_) {
    return FailedPreconditionError(
        "This video exporter has already been Run. Please Create a new "
        "instance and try again.");
  }
  has_been_run_ = true;

  // Start the writer in a background thread.
  StartWriter();

  // The main thread is the receiver thread.
  // Repeatedly fetches packets and feeds the decoder.
  Status receiver_status = OkStatus();
  while (!writer_signal_->IsCompleted()) {
    // Get a packet from the stream. Decide if it is EOS.
    Packet p;
    if (!packet_receiver_queue_->TryPop(p, options_.receiver_timeout)) {
      receiver_status = DeadlineExceededError(absl::StrFormat(
          "Failed to receive a packet within the specified timeout (%s).",
          FormatDuration(options_.receiver_timeout)));
      break;
    }
    if (IsEos(p)) {
      break;
    }

    // Convert the packet to a gstreamer buffer and feed it to yield raw images.
    auto gstreamer_buffer_statusor = ToGstreamerBuffer(std::move(p));
    if (!gstreamer_buffer_statusor.ok()) {
      receiver_status = UnknownError(absl::StrFormat(
          "Failed to convert the packet to a gstreamer buffer: %s",
          gstreamer_buffer_statusor.status().message()));
      break;
    }
    auto gstreamer_buffer = std::move(gstreamer_buffer_statusor).ValueOrDie();

    auto status = raw_image_yielder_->Feed(gstreamer_buffer);
    if (!status.ok()) {
      receiver_status = UnknownError(absl::StrFormat(
          "Failed to Feed the data for raw image conversion: %s",
          status.message()));
      break;
    }
  }

  // Stop the writer thread.
  auto stop_status = StopWriter();
  if (!stop_status.ok()) {
    LOG(ERROR) << stop_status;
  }

  // Log and return errors.
  auto writer_status = writer_signal_->GetStatus();
  if (!writer_status.ok()) {
    LOG(ERROR) << writer_status;
  }
  if (!receiver_status.ok()) {
    LOG(ERROR) << receiver_status;
  }
  if (!stop_status.ok() || !writer_status.ok() || !receiver_status.ok()) {
    return UnknownError("The Run did not complete sucessfully.");
  }

  return OkStatus();
}

GstreamerVideoExporter::~GstreamerVideoExporter() {
  auto stop_status = StopWriter();
  if (!stop_status.ok()) {
    LOG(ERROR) << stop_status;
  }
}

// --------------------------------------------------------------------
// Writer implementation.

void GstreamerVideoExporter::StartWriter() {
  if (writer_thread_.joinable()) {
    return;
  }
  writer_signal_ = std::make_unique<WriterSignal>();
  writer_signal_->Start();
  writer_thread_ = std::thread(&GstreamerVideoExporter::WriterWork, this);
  return;
}

Status GstreamerVideoExporter::StopWriter() {
  if (!writer_thread_.joinable()) {
    return OkStatus();
  }
  auto status = raw_image_yielder_->SignalEOS();
  if (!status.ok()) {
    writer_thread_.detach();
    return InternalError(absl::StrFormat(
        "Could not deliver end-of-stream to the writer: %s", status.message()));
  } else {
    if (!writer_signal_->WaitUntilCompleted(
            options_.writer_finalization_deadline)) {
      writer_thread_.detach();
      return UnknownError(
          "The writer did not finalize within the given deadline.");
    } else {
      writer_thread_.join();
    }
  }
  return OkStatus();
}

void GstreamerVideoExporter::WriterWork() {
  bool start_new_file = true;
  for (int file_index = 0; start_new_file; ++file_index) {
    std::unique_ptr<GstreamerVideoWriter> video_writer = nullptr;
    for (int image_index = 0; image_index < options_.max_frames_per_file;
         ++image_index) {
      // Get a new element from the decoder. Quit if it indicates that the
      // stream has reached the end.
      StatusOr<RawImage> raw_image_statusor;
      raw_image_pcqueue_->Pop(raw_image_statusor);
      if (raw_image_statusor.status().code() == StatusCode::kNotFound) {
        start_new_file = false;
        break;
      }

      // Get the raw image and write it into the current video file.
      auto raw_image_gstreamer_buffer_statusor =
          ToGstreamerBuffer(std::move(raw_image_statusor).ValueOrDie());
      if (!raw_image_gstreamer_buffer_statusor.ok()) {
        writer_signal_->SetStatus(UnknownError(absl::StrFormat(
            "Could not convert a raw image into a gstreamer buffer: %s",
            raw_image_gstreamer_buffer_statusor.status().message())));
        start_new_file = false;
        break;
      }
      auto raw_image_gstreamer_buffer =
          std::move(raw_image_gstreamer_buffer_statusor).ValueOrDie();

      if (video_writer == nullptr) {
        GstreamerVideoWriter::Options video_writer_options;
        // TODO: Name the files according to server conventions.
        video_writer_options.filename =
            absl::StrFormat("%s-%d.mp4", options_.file_prefix, file_index);
        video_writer_options.caps_string =
            raw_image_gstreamer_buffer.get_caps();
        auto video_writer_statusor =
            GstreamerVideoWriter::Create(video_writer_options);
        if (!video_writer_statusor.ok()) {
          writer_signal_->SetStatus(InternalError(
              absl::StrFormat("Failed to create a new video writer: %s",
                              video_writer_statusor.status().message())));
          start_new_file = false;
          break;
        }
        video_writer = std::move(video_writer_statusor).ValueOrDie();
      }

      auto status = video_writer->Put(raw_image_gstreamer_buffer);
      if (!status.ok()) {
        writer_signal_->SetStatus(UnknownError(absl::StrFormat(
            "Failed to write a raw image: %s", status.message())));
        start_new_file = false;
        break;
      }
    }
  }
  writer_signal_->End();
  return;
}

}  // namespace aistreams
