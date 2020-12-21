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

#include "aistreams/gstreamer/gstreamer_video_writer.h"

#include "absl/strings/str_format.h"
#include "absl/strings/str_join.h"
#include "aistreams/gstreamer/type_utils.h"
#include "aistreams/port/canonical_errors.h"
#include "aistreams/port/logging.h"
#include "aistreams/port/status.h"
#include "aistreams/port/status_macros.h"

namespace aistreams {

namespace {

Status ValidateOptions(const GstreamerVideoWriter::Options& options) {
  if (options.file_path.empty()) {
    return InvalidArgumentError(
        "You must supply the name of the output video file");
  }

  if (options.caps_string.empty()) {
    return InvalidArgumentError(
        "You must supply the expected caps string of the incoming gstreamer "
        "buffers");
  }
  return OkStatus();
}

StatusOr<std::string> AssembleGstreamerPipeline(
    const GstreamerVideoWriter::Options& options) {
  std::vector<std::string> pipeline_elements;
  pipeline_elements.push_back("decodebin");
  pipeline_elements.push_back("videoconvert");
  pipeline_elements.push_back("x264enc");
  pipeline_elements.push_back("mp4mux");
  pipeline_elements.push_back(
      absl::StrFormat("filesink location=%s", options.file_path));
  return absl::StrJoin(pipeline_elements, " ! ");
}

}  // namespace

GstreamerVideoWriter::GstreamerVideoWriter(const Options& options)
    : options_(options) {}

StatusOr<std::unique_ptr<GstreamerVideoWriter>> GstreamerVideoWriter::Create(
    const Options& options) {
  AIS_RETURN_IF_ERROR(ValidateOptions(options));

  auto video_writer = std::make_unique<GstreamerVideoWriter>(options);
  auto status = video_writer->Initialize();
  if (!status.ok()) {
    LOG(ERROR) << status;
    return InternalError("Failed to Initialize the GstreamerVideoWriter");
  }
  return video_writer;
}

Status GstreamerVideoWriter::Initialize() {
  // Assemble the main gstreamer processing pipeline string.
  auto pipeline_string_statusor = AssembleGstreamerPipeline(options_);
  if (!pipeline_string_statusor.ok()) {
    return pipeline_string_statusor.status();
  }
  auto pipeline_string = std::move(pipeline_string_statusor).ValueOrDie();

  // Create a GstreamerRunner that can be fed.
  GstreamerRunner::Options gstreamer_runner_options;
  gstreamer_runner_options.appsrc_caps_string = options_.caps_string;
  gstreamer_runner_options.processing_pipeline_string = pipeline_string;

  auto gstreamer_runner_statusor =
      GstreamerRunner::Create(gstreamer_runner_options);
  if (!gstreamer_runner_statusor.ok()) {
    LOG(ERROR) << gstreamer_runner_statusor.status();
    return UnknownError("Failed to create the GstreamerRunner");
  }
  gstreamer_runner_ = std::move(gstreamer_runner_statusor).ValueOrDie();

  return OkStatus();
}

Status GstreamerVideoWriter::Put(const GstreamerBuffer& gstreamer_buffer) {
  return gstreamer_runner_->Feed(gstreamer_buffer);
}

}  // namespace aistreams
