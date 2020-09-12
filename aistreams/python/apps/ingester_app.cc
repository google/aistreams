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

// TODO: add SSL configurations.
ABSL_FLAG(std::string, target_address, "localhost:50051",
          "Address (ip:port) to the AI Streams instance.");
ABSL_FLAG(std::string, stream_name, "", "Name of the stream to receiver from.");
ABSL_FLAG(std::string, source_uri, "", "The uri of the input data source.");
ABSL_FLAG(bool, loop_playback, false,
          "Whether to loop/repeat the source when it reaches the end.");

namespace aistreams {

void RunIngester() {
  // Configure the ingester.
  IngesterOptions options;
  options.connection_options.target_address =
      absl::GetFlag(FLAGS_target_address);
  options.target_stream_name = absl::GetFlag(FLAGS_stream_name);
  std::string source_uri = absl::GetFlag(FLAGS_source_uri);
  bool loop_playback = absl::GetFlag(FLAGS_loop_playback);

  // Run the ingester.
  int playback_count = 1;
  do {
    LOG(INFO) << "Starting to play the " << playback_count << "'th iteration";
    auto status = Ingest(options, source_uri);
    if (!status.ok()) {
      LOG(ERROR) << status;
      break;
    }
    ++playback_count;
  } while (loop_playback);
  return;
}

}  // namespace aistreams

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  absl::ParseCommandLine(argc, argv);
  aistreams::RunIngester();
  return 0;
}
