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

Status EOSStatus() { return Status(StatusCode::kNotFound, "Reached EOS"); }

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
  // Create a GstreamerRunner with a generic decoding pipeline.
  GstreamerRunner::Options gstreamer_runner_options;
  gstreamer_runner_options.appsrc_caps_string = options_.caps_string;
  gstreamer_runner_options.processing_pipeline_string = kGenericDecodeString;
  if (options_.callback) {
    gstreamer_runner_options.receiver_callback =
        [this](GstreamerBuffer gstreamer_buffer) -> Status {
      auto raw_image_status_or = ToRawImage(std::move(gstreamer_buffer));
      options_.callback(std::move(raw_image_status_or));
      return OkStatus();
    };
  }

  auto gstreamer_runner_statusor =
      GstreamerRunner::Create(gstreamer_runner_options);
  if (!gstreamer_runner_statusor.ok()) {
    LOG(ERROR) << gstreamer_runner_statusor.status();
    return UnknownError("Failed to create the GstreamerRunner");
  }
  gstreamer_runner_ = std::move(gstreamer_runner_statusor).ValueOrDie();

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
  gstreamer_runner_.reset(nullptr);

  // Deliver EOS. Ignore any callback errors.
  if (options_.callback) {
    auto status = options_.callback(EOSStatus());
    if (!status.ok()) {
      LOG(ERROR) << status;
    }
  }
  return OkStatus();
}

}  // namespace aistreams
