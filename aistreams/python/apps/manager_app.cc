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
#include "absl/strings/str_format.h"
#include "aistreams/cc/aistreams.h"

namespace {

// Management operations.
enum class Operation {
  kCreateStream = 0,
  kListStreams,
  kDeleteStream,
  kNumOps,
};

constexpr const char* kOpNames[] = {"CreateStream", "ListStreams",
                                    "DeleteStream"};

std::string OpNameHelpString() {
  std::string help_string;
  int n = static_cast<int>(Operation::kNumOps);
  for (int i = 0; i < n; ++i) {
    help_string += absl::StrFormat("%d: %s, ", i, kOpNames[i]);
  }
  help_string.pop_back();
  help_string.pop_back();
  return help_string;
}
}  // namespace

ABSL_FLAG(std::string, target_address, "",
          "Address (ip:port) to the AI Streams instance.");
ABSL_FLAG(std::string, stream_name, "", "Name of the stream to receiver from.");
ABSL_FLAG(bool, use_insecure_channel, true, "Use insecure channel.");
ABSL_FLAG(std::string, ssl_root_cert_path, "",
          "The path to the ssl root certificate.");
ABSL_FLAG(int, op_id, -1,
          absl::StrFormat("Management operation ID. %s.", OpNameHelpString()));
ABSL_FLAG(bool, use_google_managed_service, true,
          "Use google managed service.");

ABSL_FLAG(std::string, project, "", "The project hosting management server.");
ABSL_FLAG(std::string, location, "us-central1",
          "The location of the management server.");
ABSL_FLAG(std::string, cluster, "",
          "The cluster hosting the management server.");
namespace aistreams {

StatusOr<std::unique_ptr<StreamManager>> CreateStreamManager() {
  StreamManagerConfig config;
  const auto target_address = absl::GetFlag(FLAGS_target_address);
  if (target_address.empty()) {
    return InvalidArgumentError("Target address cannot be empty.");
  }

  // Check if the configuration is for onprem stream management service or
  // Google managed stream management service.
  if (!absl::GetFlag(FLAGS_use_google_managed_service)) {
    LOG(INFO) << "Creating an On-Prem StreamManager.";
    auto onprem_config = config.mutable_stream_manager_onprem_config();
    onprem_config->set_target_address(target_address);
    onprem_config->set_use_insecure_channel(
        absl::GetFlag(FLAGS_use_insecure_channel));
    onprem_config->set_ssl_root_cert_path(
        absl::GetFlag(FLAGS_ssl_root_cert_path));
    onprem_config->set_wait_for_ready(true);
    onprem_config->mutable_timeout()->set_seconds(google::protobuf::kint64max);
  } else {
    LOG(INFO) << "Creating a StreamManager for the Google managed service.";
    auto managed_config = config.mutable_stream_manager_managed_config();
    managed_config->set_target_address(target_address);
    managed_config->set_project(absl::GetFlag(FLAGS_project));
    managed_config->set_location(absl::GetFlag(FLAGS_location));
    managed_config->set_cluster(absl::GetFlag(FLAGS_cluster));
  }
  return StreamManagerFactory::CreateStreamManager(config);
}

void CreateStream() {
  auto manager_statusor = CreateStreamManager();
  if (!manager_statusor.ok()) {
    LOG(ERROR) << "Failed to create StreamManager. "
               << manager_statusor.status();
    return;
  }

  auto manager = std::move(manager_statusor).ValueOrDie();
  Stream stream;
  const auto stream_name = absl::GetFlag(FLAGS_stream_name);
  stream.set_name(stream_name);
  auto stream_statusor = manager->CreateStream(stream);
  if (!stream_statusor.ok()) {
    LOG(ERROR) << "Failed to call CreateStream(). " << stream_statusor.status();
  } else {
    LOG(INFO) << "Successfully created stream " << stream_name;
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
  if (streams.empty()) {
    LOG(INFO) << "No stream found.\n";
  } else {
    LOG(INFO) << absl::StrFormat("List (%d) streams: \n", streams.size());
    for (const auto& stream : streams) {
      LOG(INFO) << stream.name() << "\n";
    }
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
    LOG(INFO) << "Successfully deleted the stream " << stream_name;
  }
}
}  // namespace aistreams

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  absl::ParseCommandLine(argc, argv);
  ::Operation op = static_cast<::Operation>(absl::GetFlag(FLAGS_op_id));
  absl::flat_hash_map<::Operation, std::function<void()>> registry{
      {::Operation::kCreateStream, aistreams::CreateStream},
      {::Operation::kListStreams, aistreams::ListStreams},
      {::Operation::kDeleteStream, aistreams::DeleteStream}};
  auto it = registry.find(op);
  if (it == registry.end()) {
    LOG(ERROR) << absl::StrFormat("Invalid op id (%d). Choices are %s", op,
                                  OpNameHelpString());
  } else {
    (it->second)();
  }
  return 0;
}
