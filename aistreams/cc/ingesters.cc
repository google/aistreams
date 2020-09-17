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

#include "aistreams/cc/ingesters.h"

#include <regex>
#include <string>

#include "absl/strings/str_format.h"
#include "absl/strings/str_join.h"
#include "absl/time/time.h"
#include "aistreams/cc/aistreams_lite.h"
#include "aistreams/gstreamer/gst-plugins/aissink_cli_builder.h"
#include "aistreams/gstreamer/gstreamer_utils.h"
#include "aistreams/port/logging.h"
#include "aistreams/port/status.h"
#include "aistreams/port/statusor.h"
#include "aistreams/util/file_helpers.h"

namespace aistreams {

namespace {

namespace {

std::string SetPluginParam(absl::string_view parameter_name,
                           absl::string_view value) {
  if (value.empty()) {
    value = "\"\"";
  }
  return absl::StrFormat("%s=%s", parameter_name, value);
}

}  // namespace

bool HasProtocolPrefix(const std::string& source_uri) {
  std::regex re("^.*://");
  return std::regex_search(source_uri, re);
}

// Decide the gstreamer plugin used to accept the input.
//
// The input source uri is assumed to be a file path if there is no uri prefix.
std::string DecideInputPlugin(const std::string& source_uri) {
  if (HasProtocolPrefix(source_uri)) {
    return absl::StrFormat("urisourcebin %s",
                           SetPluginParam("uri", source_uri));
  } else {
    return absl::StrFormat("filesrc %s",
                           SetPluginParam("location", source_uri));
  }
}

// Decide whether a local transcode is required.
bool IsTranscodeRequired(const IngesterOptions& options) {
  // This holds if the user explicitly asks to transcode into one of the
  // non-native types.
  if (options.send_codec != IngesterOptions::SendCodec::kNative) {
    return true;
  }

  // Resizing supported only with raw images, so you have to decode before you
  // can do this.
  if (options.resize_height > 0 || options.resize_width > 0) {
    return true;
  }
  return false;
}

// Decide the videoscale plugin configuration if resizing is needed.
//
// Returns empty string if no resizing is called for.
std::string DecideResizePlugin(const IngesterOptions& options) {
  std::string videoscale_config;
  int resize_height = options.resize_height;
  int resize_width = options.resize_width;
  if (resize_width > 0 || resize_height > 0) {
    videoscale_config = "videoscale ! video/x-raw";
    if (resize_width > 0) {
      videoscale_config += absl::StrFormat(",width=%d", resize_width);
    }
    if (resize_height > 0) {
      videoscale_config += absl::StrFormat(",height=%d", resize_height);
    }
  }
  return videoscale_config;
}

// Decide which gstreamer plugin to transcode into before it is sent.
//
// Returns an empty string if no transcoding is necessary.
std::string DecideCodecPlugins(const IngesterOptions& options) {
  if (!IsTranscodeRequired(options)) {
    return "";
  }

  IngesterOptions::SendCodec send_codec = options.send_codec;
  if (send_codec == IngesterOptions::SendCodec::kNative) {
    LOG(WARNING) << "A transcoding is required but you did not specify a "
                    "sending codec. Defaulting to H264";
    send_codec = IngesterOptions::SendCodec::kH264;
  }

  switch (send_codec) {
    case IngesterOptions::SendCodec::kJpeg:
      return "jpegenc";
    case IngesterOptions::SendCodec::kRawRgb:
      return "videoconvert ! video/x-raw,format=RGB";
    default:
      return "h264enc";
  }
}

// Decide what the full gst pipeline should be given the options and source uri.
StatusOr<std::string> DecideGstLaunchPipeline(const IngesterOptions& options,
                                              const std::string& source_uri) {
  std::vector<std::string> gst_pipeline;

  // This is the plugin that accepts the source uri into the pipeline.
  //
  // It normalizes all streaming protocols, video container formats, and outputs
  // buffer types that are suitable to be passed for codec
  // determination/processing.
  std::string source_plugin = DecideInputPlugin(source_uri);
  gst_pipeline.push_back(source_plugin);

  // If a transcode is not required, then directly pass the natively encoded
  // buffers into aissink. Otherwise, decode, resize, and re-encoding into the
  // final sending format.
  if (!IsTranscodeRequired(options)) {
    gst_pipeline.push_back("parsebin");
  } else {
    gst_pipeline.push_back("decodebin");
    auto resize_plugin = DecideResizePlugin(options);
    if (!resize_plugin.empty()) {
      gst_pipeline.push_back(resize_plugin);
    }
    auto codec_plugin = DecideCodecPlugins(options);
    if (!codec_plugin.empty()) {
      gst_pipeline.push_back(codec_plugin);
    }
  }

  // Configure aissink.
  AissinkCliBuilder aissink_cli_builder;
  auto aissink_plugin_statusor =
      aissink_cli_builder
          .SetTargetAddress(options.connection_options.target_address)
          .SetStreamName(options.target_stream_name)
          .SetSslOptions(options.connection_options.ssl_options)
          .Finalize();
  if (!aissink_plugin_statusor.ok()) {
    LOG(ERROR) << aissink_plugin_statusor.status();
    return InvalidArgumentError(
        "Could not get a valid configuration for aissink");
  }
  gst_pipeline.push_back(aissink_plugin_statusor.ValueOrDie());

  return absl::StrJoin(gst_pipeline, " ! ");
}

}  // namespace

Status Ingest(const IngesterOptions& options, absl::string_view source_uri) {
  // If the given source uri is not protocol prefixed, assume that it is a file
  // name and explicitly check that it exists.
  //
  // We do this explicitly to make this very common mistake more transparent to
  // the user. Gstreamer can technically detect this, but the message tends to
  // be buried.
  if (!HasProtocolPrefix(std::string(source_uri))) {
    auto status = file::Exists(source_uri);
    if (!status.ok()) {
      return InvalidArgumentError(
          absl::StrFormat("The file \"%s\" could not be accessed: %s",
                          source_uri, status.message()));
    }
  }

  // Decide what the gst pipeline should be based on the options.
  auto gst_pipeline_statusor =
      DecideGstLaunchPipeline(options, std::string(source_uri));
  if (!gst_pipeline_statusor.ok()) {
    LOG(ERROR) << gst_pipeline_statusor.status();
    return InvalidArgumentError("Could not decide on a gst pipeline to launch");
  }
  auto gst_pipeline = std::move(gst_pipeline_statusor).ValueOrDie();
  LOG(INFO) << absl::StrFormat("Will run the gst pipeline\n  %s", gst_pipeline);

  // Launch the pipeline.
  auto status = GstLaunchPipeline(gst_pipeline);
  if (!status.ok()) {
    LOG(ERROR) << status;
    return UnknownError(absl::StrFormat(
        "Failed to launch the gst pipeline:\n  %s", gst_pipeline));
  }
  return OkStatus();
}

}  // namespace aistreams
