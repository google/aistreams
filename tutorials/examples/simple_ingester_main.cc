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

// This example shows how you can programmatically ingest a video source.
// It assumes that you already have a stream created in the server; if you don't
// know how to do this, please refer to the README.md (e.g. using aisctl).

#include <memory>
#include <string>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/strings/str_format.h"
#include "absl/time/time.h"
#include "aistreams/cc/aistreams.h"
#include "aistreams/port/logging.h"
#include "aistreams/port/status.h"
#include "aistreams/port/statusor.h"

ABSL_FLAG(std::string, target_address, "",
          "Address (ip:port) to the data ingress.");
ABSL_FLAG(std::string, ssl_root_cert_path, "",
          "Path to the data ingress' ssl certificate.");
ABSL_FLAG(bool, authenticate_with_google, true,
          "Set to true if you are sending to a stream on the Google managed "
          "service; false otherwise.");
ABSL_FLAG(std::string, stream_name, "",
          "Name of the stream to ingest videos into.");
ABSL_FLAG(std::string, source_uri, "",
          "The uri of the input data source; e.g. my-video.mp4.");

namespace aistreams {

Status RunIngester() {
  // Options to configure the ingester.
  //
  // See aistreams/cc/ingester.h for more details.
  IngesterOptions ingester_options;

  // Configure the connection to the data ingress.
  //
  // For Google managed service streams, you can get the required information by
  // using `aisctl managed list`.
  //
  // For on-prem streams, you will have to ask your sysadmin for the details
  // (we work with them to make sure you have all of the items here).
  //
  // You can see aistreams/base/connection_options.h for more details about
  // these parameters. But these are really the ones you need to pay attention
  // to:
  //
  // `target_address`: This is the data ingress' ip:port.
  // `ssl_root_cert_path`: This is the path to the ssl certificate.
  // `authenticate_with_google`: Set this to true if you are connecting to a
  //     stream on the Google managed service. Set to false otherwise.
  ConnectionOptions connection_options;
  connection_options.target_address = absl::GetFlag(FLAGS_target_address);
  connection_options.ssl_options.ssl_root_cert_path =
      absl::GetFlag(FLAGS_ssl_root_cert_path);
  connection_options.authenticate_with_google =
      absl::GetFlag(FLAGS_authenticate_with_google);
  ingester_options.connection_options = connection_options;

  // Specify the stream to ingest into.
  ingester_options.target_stream_name = absl::GetFlag(FLAGS_stream_name);

  // Ingest a video source. Blocks until the source is depleted.
  std::string source_uri = absl::GetFlag(FLAGS_source_uri);
  auto status = Ingest(ingester_options, source_uri);
  if (!status.ok()) {
    LOG(ERROR) << status;
    return UnknownError("The Ingestion did not exit normally");
  }

  return OkStatus();
}

}  // namespace aistreams

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  absl::ParseCommandLine(argc, argv);
  auto status = aistreams::RunIngester();
  if (!status.ok()) {
    LOG(ERROR) << status;
    return EXIT_FAILURE;
  }
  return 0;
}
