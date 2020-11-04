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
  kCreateCluster,
  kListClusters,
  kDeleteCluster,
  kGetCluster,
  kNumOps,
};

constexpr const char* kOpNames[] = {
    "CreateStream", "ListStreams",   "DeleteStream", "CreateCluster",
    "ListClusters", "DeleteCluster", "GetCluster"};

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
ABSL_FLAG(std::string, ssl_domain_name, "aistreams.googleapis.com",
          "The expected ssl domain name of the service.");
ABSL_FLAG(int, op_id, -1,
          absl::StrFormat("Management operation ID. %s.", OpNameHelpString()));
ABSL_FLAG(bool, use_google_managed_service, true,
          "Use google managed service.");
ABSL_FLAG(std::string, project, "", "The project hosting management server.");
ABSL_FLAG(std::string, location, "us-central1",
          "The location of the management server.");
ABSL_FLAG(std::string, cluster_name, "",
          "The cluster hosting the management server.");
ABSL_FLAG(int, stream_retention_seconds, 86400,
          "The retention period for the stream.");

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
    auto onprem_config = config.mutable_stream_manager_onprem_config();
    onprem_config->set_target_address(target_address);
    onprem_config->set_use_insecure_channel(
        absl::GetFlag(FLAGS_use_insecure_channel));
    onprem_config->set_ssl_root_cert_path(
        absl::GetFlag(FLAGS_ssl_root_cert_path));
    onprem_config->set_ssl_domain_name(absl::GetFlag(FLAGS_ssl_domain_name));
    onprem_config->set_wait_for_ready(true);
    onprem_config->mutable_timeout()->set_seconds(google::protobuf::kint64max);
  } else {
    auto managed_config = config.mutable_stream_manager_managed_config();
    const auto project_id = absl::GetFlag(FLAGS_project);
    if (project_id.empty()) {
      return InvalidArgumentError("Project id cannot be empty.");
    }
    const auto location = absl::GetFlag(FLAGS_location);
    if (location.empty()) {
      return InvalidArgumentError("Location cannot be empty.");
    }
    const auto cluster_name = absl::GetFlag(FLAGS_cluster_name);
    if (cluster_name.empty()) {
      return InvalidArgumentError("Cluster name cannot be empty.");
    }

    managed_config->set_target_address(target_address);
    managed_config->set_project(project_id);
    managed_config->set_location(location);
    managed_config->set_cluster(cluster_name);
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
  if (stream_name.empty()) {
    LOG(ERROR) << "Stream name cannot be empty.";
    return;
  }
  const auto stream_retention_seconds =
      absl::GetFlag(FLAGS_stream_retention_seconds);

  stream.set_name(stream_name);
  stream.mutable_retention_period()->set_seconds(stream_retention_seconds);
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
    LOG(INFO) << "No streams found.\n";
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
  if (stream_name.empty()) {
    LOG(ERROR) << "Stream name cannot be empty.";
    return;
  }
  auto status = manager->DeleteStream(stream_name);
  if (!status.ok()) {
    LOG(ERROR) << "Failed to call DeleteStream(). " << status;
  } else {
    LOG(INFO) << "Successfully deleted the stream " << stream_name;
  }
}

StatusOr<std::unique_ptr<ClusterManager>> CreateClusterManager() {
  if (!absl::GetFlag(FLAGS_use_google_managed_service)) {
    return UnimplementedError(
        "ClusterManager is not availabe for on-prem management server.");
  }

  ClusterManagerConfig config;
  const auto target_address = absl::GetFlag(FLAGS_target_address);
  if (target_address.empty()) {
    return InvalidArgumentError("Target address cannot be empty.");
  }
  const auto project_id = absl::GetFlag(FLAGS_project);
  if (project_id.empty()) {
    return InvalidArgumentError("Project id cannot be empty.");
  }
  const auto location = absl::GetFlag(FLAGS_location);
  if (location.empty()) {
    return InvalidArgumentError("Location cannot be empty.");
  }
  config.set_target_address(target_address);
  config.set_project(project_id);
  config.set_location(location);
  return ClusterManagerFactory::CreateClusterManager(config);
}

void CreateCluster() {
  auto manager_statusor = CreateClusterManager();
  if (!manager_statusor.ok()) {
    LOG(ERROR) << "Failed to create ClusterManager. "
               << manager_statusor.status();
    return;
  }

  auto manager = std::move(manager_statusor).ValueOrDie();
  Cluster cluster;
  const auto cluster_name = absl::GetFlag(FLAGS_cluster_name);
  if (cluster_name.empty()) {
    LOG(ERROR) << "Cluster name cannot be empty.";
    return;
  }
  cluster.set_name(cluster_name);
  auto cluster_statusor = manager->CreateCluster(cluster);
  if (!cluster_statusor.ok()) {
    LOG(ERROR) << "Failed to call CreateCluster(). "
               << cluster_statusor.status();
  } else {
    LOG(INFO) << "Successfully created cluster " << cluster_name;
  }
}

void ListClusters() {
  auto manager_statusor = CreateClusterManager();
  if (!manager_statusor.ok()) {
    LOG(ERROR) << "Failed to create ClusterManager. "
               << manager_statusor.status();
    return;
  }

  auto manager = std::move(manager_statusor).ValueOrDie();
  auto clusters_statusor = manager->ListClusters();
  if (!clusters_statusor.ok()) {
    LOG(ERROR) << "Failed to call ListClusters(). "
               << clusters_statusor.status();
    return;
  }

  auto clusters = std::move(clusters_statusor).ValueOrDie();
  if (clusters.empty()) {
    LOG(INFO) << "No clusters found.\n";
  } else {
    LOG(INFO) << absl::StrFormat("List (%d) clusters: \n", clusters.size());
    for (const auto& cluster : clusters) {
      LOG(INFO) << cluster.name() << "\t" << cluster.service_endpoint() << "\n"
                << cluster.certificate() << "\n";
    }
  }
}

void DeleteCluster() {
  auto manager_statusor = CreateClusterManager();
  if (!manager_statusor.ok()) {
    LOG(ERROR) << "Failed to create ClusterManager. "
               << manager_statusor.status();
    return;
  }

  auto manager = std::move(manager_statusor).ValueOrDie();
  const auto cluster_name = absl::GetFlag(FLAGS_cluster_name);
  if (cluster_name.empty()) {
    LOG(ERROR) << "Cluster name cannot be empty.";
    return;
  }
  auto status = manager->DeleteCluster(cluster_name);
  if (!status.ok()) {
    LOG(ERROR) << "Failed to call DeleteCluster(). " << status;
  } else {
    LOG(INFO) << "Successfully deleted the cluster " << cluster_name;
  }
}

void GetCluster() {
  auto manager_statusor = CreateClusterManager();
  if (!manager_statusor.ok()) {
    LOG(ERROR) << "Failed to create ClusterManager. "
               << manager_statusor.status();
    return;
  }

  auto manager = std::move(manager_statusor).ValueOrDie();
  const auto cluster_name = absl::GetFlag(FLAGS_cluster_name);
  if (cluster_name.empty()) {
    LOG(ERROR) << "Cluster name cannot be empty.";
    return;
  }
  auto cluster_statusor = manager->GetCluster(cluster_name);
  if (!cluster_statusor.ok()) {
    LOG(ERROR) << "Failed to call GetCluster(). " << cluster_statusor.status();
  } else {
    auto cluster = std::move(cluster_statusor).ValueOrDie();
    LOG(INFO) << cluster.name() << "\t" << cluster.service_endpoint() << "\n"
              << cluster.certificate() << "\n";
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
      {::Operation::kDeleteStream, aistreams::DeleteStream},
      {::Operation::kCreateCluster, aistreams::CreateCluster},
      {::Operation::kListClusters, aistreams::ListClusters},
      {::Operation::kDeleteCluster, aistreams::DeleteCluster},
      {::Operation::kGetCluster, aistreams::GetCluster}};
  auto it = registry.find(op);
  if (it == registry.end()) {
    LOG(ERROR) << absl::StrFormat("Invalid op id (%d). Choices are %s", op,
                                  OpNameHelpString());
  } else {
    (it->second)();
  }
  return 0;
}
