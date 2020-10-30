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
#include <string>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/strings/str_format.h"
#include "absl/time/time.h"
#include "aistreams/cc/aistreams.h"
#include "aistreams/port/logging.h"
#include "aistreams/port/status.h"
#include "aistreams/port/statusor.h"

ABSL_FLAG(std::string, target_address, "localhost:50051",
          "Address (ip:port) to the AI Streams instance.");
ABSL_FLAG(bool, authenticate_with_google, false,
          "Set to true for the managed service; otherwise false.");
ABSL_FLAG(std::string, stream_name, "", "Name of the stream to send to.");
ABSL_FLAG(std::string, source_uri, "", "The uri of the input data source.");
ABSL_FLAG(bool, use_insecure_channel, true, "Use an insecure channel.");
ABSL_FLAG(std::string, ssl_domain_name, "aistreams.googleapis.com",
          "The expected ssl domain name of the service.");
ABSL_FLAG(std::string, ssl_root_cert_path, "",
          "The path to the ssl root certificate.");
ABSL_FLAG(bool, loop, false,
          "Whether to loop/repeat the source when it reaches the end.");
ABSL_FLAG(double, trace_probability, 0,
          "The probability to start trace for each packet.");

namespace aistreams {

void RunIngester() {
  // Configure the ingester.
  IngesterOptions options;
  options.connection_options.target_address =
      absl::GetFlag(FLAGS_target_address);
  options.connection_options.authenticate_with_google =
      absl::GetFlag(FLAGS_authenticate_with_google);
  options.connection_options.ssl_options.use_insecure_channel =
      absl::GetFlag(FLAGS_use_insecure_channel);
  options.connection_options.ssl_options.ssl_domain_name =
      absl::GetFlag(FLAGS_ssl_domain_name);
  options.connection_options.ssl_options.ssl_root_cert_path =
      absl::GetFlag(FLAGS_ssl_root_cert_path);
  options.target_stream_name = absl::GetFlag(FLAGS_stream_name);
  std::string source_uri = absl::GetFlag(FLAGS_source_uri);
  options.trace_probability = absl::GetFlag(FLAGS_trace_probability);
  bool loop = absl::GetFlag(FLAGS_loop);

  // Run the ingester.
  int loop_count = 1;
  do {
    LOG(INFO) << "Starting to play the " << loop_count << "'th iteration";
    auto status = Ingest(options, source_uri);
    if (!status.ok()) {
      LOG(ERROR) << status;
      break;
    }
    ++loop_count;
  } while (loop);
  return;
}

}  // namespace aistreams

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  absl::ParseCommandLine(argc, argv);
  aistreams::RunIngester();
  return 0;
}
