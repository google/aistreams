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

#include <memory>
#include <regex>
#include <string>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_join.h"
#include "absl/time/time.h"
#include "aistreams/base/wrappers/receivers.h"
#include "aistreams/gstreamer/gstreamer_video_exporter.h"
#include "aistreams/port/canonical_errors.h"
#include "aistreams/port/logging.h"
#include "aistreams/port/status.h"
#include "aistreams/port/statusor.h"

// Options that configure video outputs.
ABSL_FLAG(int, max_frames_per_file, 200,
          "The maximum number of video frames per file.");
ABSL_FLAG(std::string, output_dir, "",
          "The directory to output local video files.");
ABSL_FLAG(std::string, file_prefix, "",
          "Optional prefix for the output video files.");

// Options to configure video uploads to GCS.
ABSL_FLAG(bool, upload_to_gcs, false,
          "Whether to upload chunked videos to GCS.");
ABSL_FLAG(bool, keep_local, false,
          "If uploading to GCS, whether to keep a local copy.");
ABSL_FLAG(std::string, gcs_bucket_name, "",
          "The name of the GCS bucket to upload to.");
ABSL_FLAG(std::string, gcs_object_dir, "",
          "The sub-directory of the GCS bucket to upload into.");

// Options for using a URI as the video input source.
ABSL_FLAG(bool, use_uri_source, true,
          "Use a gstreamer pipeline as the video input.");
ABSL_FLAG(std::string, source_uri, "", "The uri of the input data source.");

// Options for using a gstreamer pipeline as the video input source.
ABSL_FLAG(bool, use_gstreamer_input_source, false,
          "Use a gstreamer pipeline as the video input.");
ABSL_FLAG(std ::string, gstreamer_input_pipeline, "",
          "A gstreamer pipeline that produces video/x-raw as output.");

// Options for using streams as the video source.
ABSL_FLAG(std::string, target_address, "localhost:50051",
          "Address (ip:port) to the AI Streams instance.");
ABSL_FLAG(bool, authenticate_with_google, false,
          "Set to true for the managed service; otherwise false.");
ABSL_FLAG(std::string, stream_name, "", "Name of the stream to send to.");
ABSL_FLAG(bool, use_insecure_channel, true, "Use an insecure channel.");
ABSL_FLAG(std::string, ssl_domain_name, "aistreams.googleapis.com",
          "The expected ssl domain name of the service.");
ABSL_FLAG(std::string, ssl_root_cert_path, "",
          "The path to the ssl root certificate.");
ABSL_FLAG(int, receiver_timeout_in_sec, 15,
          "The timeout for the stream server to deliver a packet.");

// System configurations.
ABSL_FLAG(int, working_buffer_size, 100,
          "The size of the internal work buffers.");
ABSL_FLAG(int, finalization_deadline_in_sec, 5,
          "The timeout for internal workers to finalize their tasks.");

namespace aistreams {

namespace {

bool HasProtocolPrefix(const std::string& source_uri) {
  static std::regex re("^.*://");
  return std::regex_search(source_uri, re);
}

StatusOr<std::string> DecideUriGstreamerPipeline(
    const std::string& source_uri) {
  if (source_uri.empty()) {
    return InvalidArgumentError("You must supply a non-empty uri string.");
  }

  std::vector<std::string> pipeline_elements;

  // Assume it is a file when there's no protocol prefix.
  if (!HasProtocolPrefix(source_uri)) {
    pipeline_elements.push_back(
        absl::StrFormat("filesrc location=%s", source_uri));
    pipeline_elements.push_back("decodebin");
  } else {
    pipeline_elements.push_back(
        absl::StrFormat("uridecodebin uri=%s", source_uri));
  }
  return absl::StrJoin(pipeline_elements, " ! ");
}

}  // namespace

void RunChunker() {
  GstreamerVideoExporter::Options options;

  // Configure the video output.
  options.max_frames_per_file = absl::GetFlag(FLAGS_max_frames_per_file);
  options.output_dir = absl::GetFlag(FLAGS_output_dir);
  options.file_prefix = absl::GetFlag(FLAGS_file_prefix);

  // Configure video uploads.
  options.upload_to_gcs = absl::GetFlag(FLAGS_upload_to_gcs);
  options.keep_local = absl::GetFlag(FLAGS_keep_local);
  options.gcs_bucket_name = absl::GetFlag(FLAGS_gcs_bucket_name);
  options.gcs_object_dir = absl::GetFlag(FLAGS_gcs_object_dir);

  // Configure the input video source.
  options.use_gstreamer_input_source =
      absl::GetFlag(FLAGS_use_gstreamer_input_source);
  options.gstreamer_input_pipeline =
      absl::GetFlag(FLAGS_gstreamer_input_pipeline);
  bool use_uri_source = absl::GetFlag(FLAGS_use_uri_source);
  if (use_uri_source) {
    std::string source_uri = absl::GetFlag(FLAGS_source_uri);
    auto uri_gst_pipeline_statusor = DecideUriGstreamerPipeline(source_uri);
    if (!uri_gst_pipeline_statusor.ok()) {
      LOG(ERROR) << uri_gst_pipeline_statusor.status();
      return;
    }
    options.use_gstreamer_input_source = true;
    options.gstreamer_input_pipeline =
        std::move(uri_gst_pipeline_statusor).ValueOrDie();
  }

  ReceiverOptions receiver_options;
  receiver_options.connection_options.target_address =
      absl::GetFlag(FLAGS_target_address);
  receiver_options.connection_options.authenticate_with_google =
      absl::GetFlag(FLAGS_authenticate_with_google);
  receiver_options.connection_options.ssl_options.use_insecure_channel =
      absl::GetFlag(FLAGS_use_insecure_channel);
  receiver_options.connection_options.ssl_options.ssl_domain_name =
      absl::GetFlag(FLAGS_ssl_domain_name);
  receiver_options.connection_options.ssl_options.ssl_root_cert_path =
      absl::GetFlag(FLAGS_ssl_root_cert_path);
  receiver_options.stream_name = absl::GetFlag(FLAGS_stream_name);
  options.receiver_options = receiver_options;
  options.receiver_timeout =
      absl::Seconds(absl::GetFlag(FLAGS_receiver_timeout_in_sec));

  // Configure system settings.
  options.working_buffer_size = absl::GetFlag(FLAGS_working_buffer_size);
  options.finalization_deadline =
      absl::Seconds(absl::GetFlag(FLAGS_finalization_deadline_in_sec));

  // Run the video exporter.
  auto video_exporter_statusor = GstreamerVideoExporter::Create(options);
  if (!video_exporter_statusor.ok()) {
    LOG(ERROR) << "Failed to create the video exporter: "
               << video_exporter_statusor.status();
    return;
  }
  auto video_exporter = std::move(video_exporter_statusor).ValueOrDie();

  auto status = video_exporter->Run();
  if (!status.ok()) {
    LOG(ERROR) << "The video exporter did not complete normally: " << status;
  }
  LOG(INFO) << "Done.";
  return;
}

}  // namespace aistreams

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  absl::ParseCommandLine(argc, argv);
  aistreams::RunChunker();
  return 0;
}
