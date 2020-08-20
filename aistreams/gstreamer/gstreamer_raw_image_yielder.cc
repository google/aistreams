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

#include "aistreams/gstreamer/gstreamer_raw_image_yielder.h"

#include "absl/strings/str_format.h"
#include "aistreams/gstreamer/type_utils.h"
#include "aistreams/port/canonical_errors.h"
#include "aistreams/port/logging.h"
#include "aistreams/port/status.h"
#include "aistreams/port/status_macros.h"

namespace aistreams {

namespace {

constexpr char kGenericDecodeString[] =
    "decodebin ! videoconvert ! video/x-raw,format=RGB";

// TODO: Simply use the generic decoding pipeline for starters.
// Let the runner handle the checks. May want to consider doing more upfront
// checks here in the future.
GstreamerRunnerOptions ConstructGstreamerRunnerOptions(
    const GstreamerRawImageYielder::Options& options) {
  GstreamerRunnerOptions runner_options;
  runner_options.appsrc_caps_string = options.caps_string;
  runner_options.processing_pipeline_string = kGenericDecodeString;
  return runner_options;
}

Status EOSStatus() {
  return Status(StatusCode::kResourceExhausted, "Reached EOS");
}

}  // namespace

GstreamerRawImageYielder::GstreamerRawImageYielder(const Options& options)
    : options_(options) {}

GstreamerRawImageYielder::~GstreamerRawImageYielder() {
  if (!eos_signaled_) {
    SignalEOS();
  }
}

StatusOr<std::unique_ptr<GstreamerRawImageYielder>>
GstreamerRawImageYielder::Create(const Options& options) {
  auto raw_image_yielder = std::make_unique<GstreamerRawImageYielder>(options);
  auto status = raw_image_yielder->Initialize();
  if (!status.ok()) {
    LOG(ERROR) << status;
    return InternalError("Failed to Initialize the GstreamerRawImageYielder");
  }
  return raw_image_yielder;
}

Status GstreamerRawImageYielder::Initialize() {
  auto gstreamer_runner_options = ConstructGstreamerRunnerOptions(options_);
  gstreamer_runner_ =
      std::make_unique<GstreamerRunner>(gstreamer_runner_options);
  auto gstreamer_runner_receiver =
      [this](GstreamerBuffer gstreamer_buffer) -> Status {
    // No-op if callback is not supplied.
    if (!options_.callback) {
      return OkStatus();
    }

    // Otherwise, try to convert the raw image.
    // Leave the statusor interpretation to the recipient.
    auto raw_image_status_or = ToRawImage(std::move(gstreamer_buffer));
    options_.callback(std::move(raw_image_status_or));
    return OkStatus();
  };
  gstreamer_runner_->SetReceiver(gstreamer_runner_receiver);

  auto status = gstreamer_runner_->Start();
  if (!status.ok()) {
    LOG(ERROR) << status;
    return UnknownError("Failed to Start() the GstreamerRunner");
  }

  return OkStatus();
}

Status GstreamerRawImageYielder::Feed(const GstreamerBuffer& gstreamer_buffer) {
  if (eos_signaled_) {
    return FailedPreconditionError("Cannot feed after EOS is signaled");
  }
  return gstreamer_runner_->Feed(gstreamer_buffer);
}

Status GstreamerRawImageYielder::SignalEOS() {
  eos_signaled_ = true;
  auto status = gstreamer_runner_->End();
  if (!status.ok()) {
    LOG(ERROR) << status;
    return UnknownError("Failed to End() the GstreamerRunner");
  }

  // Deliver EOS. Ignore any callback errors.
  status = options_.callback(EOSStatus());
  if (!status.ok()) {
    LOG(ERROR) << status;
  }
  return OkStatus();
}

}  // namespace aistreams
