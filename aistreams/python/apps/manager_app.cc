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

#include "absl/container/flat_hash_map.h"
#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/strings/match.h"
#include "aistreams/cc/aistreams.h"

ABSL_FLAG(std::string, target_address, "",
          "Address (ip:port) to the AI Streams instance.");
ABSL_FLAG(std::string, stream_name, "", "Name of the stream to receiver from.");
ABSL_FLAG(bool, use_insecure_channel, true, "Use insecure channel.");
ABSL_FLAG(std::string, ssl_root_cert_path, "",
          "The path to the ssl root certificate.");
ABSL_FLAG(int, management_operation, -1,
          "Management operation ID which will be converted to Operation enum.");
ABSL_FLAG(bool, use_google_managed_service, true,
          "Use google managed service.");

namespace aistreams {
namespace {
// Management operations.
enum class Operation {
  kCreateStream = 0,
  kListStreams,
  kDeleteStream,
};
}  // namespace

StatusOr<std::unique_ptr<StreamManager>> CreateStreamManager() {
  StreamManagerConfig config;
  const auto target_address = absl::GetFlag(FLAGS_target_address);
  if (target_address.empty()) {
    return InvalidArgumentError("Target address cannot be empty.");
  }

  // Check if the configuration is for onprem stream management service or
  // Google managed stream management service.
  if (!absl::GetFlag(FLAGS_use_google_managed_service)) {
    LOG(INFO) << "Create StreamManager for on-prem stream management service.";
    auto onprem_config = config.mutable_stream_manager_onprem_config();
    onprem_config->set_target_address(target_address);
    onprem_config->set_use_insecure_channel(
        absl::GetFlag(FLAGS_use_insecure_channel));
    onprem_config->set_ssl_root_cert_path(
        absl::GetFlag(FLAGS_ssl_root_cert_path));
    onprem_config->set_wait_for_ready(true);
    onprem_config->mutable_timeout()->set_seconds(google::protobuf::kint64max);
  } else {
    LOG(INFO)
        << "Create StreamManager for Google managed stream management service.";
  }
  return StreamManagerFactory::CreateStreamManager(config);
}

void CreateStream() {
  auto manager_statusor = CreateStreamManager();
  if (!manager_statusor.ok()) {
    LOG(ERROR) << "Failed to create StreamManager. "
               << manager_statusor.status();
  }

  auto manager = std::move(manager_statusor).ValueOrDie();
  Stream stream;
  const auto stream_name = absl::GetFlag(FLAGS_stream_name);
  stream.set_name(stream_name);
  auto stream_statusor = manager->CreateStream(stream);
  if (!stream_statusor.ok()) {
    LOG(ERROR) << "Failed to call CreateStream(). " << stream_statusor.status();
  } else {
    LOG(INFO) << "Succeeded to create stream " << stream_name;
  }
}

void ListStreams() {
  auto manager_statusor = CreateStreamManager();
  if (!manager_statusor.ok()) {
    LOG(ERROR) << "Failed to create StreamManager. "
               << manager_statusor.status();
    return;
  }

  auto manager = std::move(manager_statusor).ValueOrDie();
  auto streams_statusor = manager->ListStreams();
  if (!streams_statusor.ok()) {
    LOG(ERROR) << "Failed to call ListStreams(). " << streams_statusor.status();
    return;
  }

  auto streams = std::move(streams_statusor).ValueOrDie();
  LOG(INFO) << "Stream Name\n";
  for (const auto& stream : streams) {
    LOG(INFO) << stream.name() << "\n";
  }
}

void DeleteStream() {
  auto manager_statusor = CreateStreamManager();
  if (!manager_statusor.ok()) {
    LOG(ERROR) << "Failed to create StreamManager. "
               << manager_statusor.status();
    return;
  }

  auto manager = std::move(manager_statusor).ValueOrDie();
  const auto stream_name = absl::GetFlag(FLAGS_stream_name);
  auto status = manager->DeleteStream(stream_name);
  if (!status.ok()) {
    LOG(ERROR) << "Failed to call DeleteStream(). " << status;
  } else {
    LOG(INFO) << "Succeed to delete stream " << stream_name;
  }
}
}  // namespace aistreams

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  absl::ParseCommandLine(argc, argv);
  aistreams::Operation op = static_cast<aistreams::Operation>(
      absl::GetFlag(FLAGS_management_operation));
  absl::flat_hash_map<aistreams::Operation, std::function<void()>> registry{
      {aistreams::Operation::kCreateStream, aistreams::CreateStream},
      {aistreams::Operation::kListStreams, aistreams::ListStreams},
      {aistreams::Operation::kDeleteStream, aistreams::DeleteStream}};
  auto it = registry.find(op);
  if (it == registry.end()) {
    LOG(ERROR) << "Invalid management operation.";
  } else {
    (it->second)();
  }
  return 0;
}