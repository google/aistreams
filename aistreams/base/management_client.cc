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

#include "aistreams/base/management_client.h"

#include "absl/strings/str_format.h"
#include "absl/strings/str_split.h"
#include "aistreams/base/util/auth_helpers.h"
#include "aistreams/base/util/grpc_helpers.h"
#include "aistreams/port/canonical_errors.h"
#include "aistreams/port/status.h"
#include "aistreams/port/status_macros.h"
#include "aistreams/proto/management.grpc.pb.h"
#include "aistreams/proto/management.pb.h"
#include "google/longrunning/operations.grpc.pb.h"
#include "google/longrunning/operations.pb.h"
#include "google/partner/aistreams/v1alpha1/aistreams.grpc.pb.h"
#include "google/partner/aistreams/v1alpha1/aistreams.pb.h"

namespace aistreams {
namespace {
constexpr char kAuthorization[] = "authorization";
constexpr char kTokenFormat[] = "Bearer %s";
constexpr char kOperationAPI[] = "longrunning.googleapis.com";
constexpr char kGrpcMetadata[] = "x-goog-request-params";
using ::google::longrunning::Operation;
using ::google::longrunning::Operations;
using ::google::partner::aistreams::v1alpha1::AIStreams;

void ReplaceServiceEndpointPort(std::string& endpoint) {
  const std::string target_port = ":80";
  auto pos = endpoint.find(target_port);
  if (pos == std::string::npos) {
    return;
  }
  endpoint.replace(pos, target_port.length(), ":443");
}

StatusOr<Operation> WaitOperation(const Operation& operation,
                                  const std::string& parent) {
  ConnectionOptions options;
  options.authenticate_with_google = true;
  options.target_address = kOperationAPI;
  auto channel = CreateGrpcChannel(options);
  if (channel == nullptr) {
    return UnknownError("Failed to create a gRPC channel");
  }
  std::unique_ptr<Operations::Stub> stub = Operations::NewStub(channel);
  if (stub == nullptr) {
    return UnknownError("Failed to create a gRPC stub");
  }
  ::google::longrunning::WaitOperationRequest request;
  ::google::longrunning::Operation response;
  std::vector<std::string> names = absl::StrSplit(operation.name(), "/");
  request.set_name(absl::StrFormat("operations/%s", names.back()));
  LOG(INFO) << request.name();
  LOG(INFO) << operation.name();
  grpc::ClientContext context;
  // Needs to add metadata. GFE needs the metadata for routing request.
  context.AddMetadata(kGrpcMetadata, absl::StrFormat("parent=%s", parent));

  auto grpc_status = stub->WaitOperation(&context, request, &response);
  if (!grpc_status.ok()) {
    LOG(ERROR) << grpc_status.error_message();
    return UnknownError("Encountered error calling RPC WaitOperation");
  }
  if (!response.done()) {
    return UnknownError("Operation is not done");
  }
  if (response.has_error()) {
    LOG(ERROR) << response.error().message();
    return UnknownError("Operation failed.");
  }
  return response;
}
}  // namespace

// Stream manager for managing streams in on-prem cluster.
class OnPremStreamManagerImpl : public StreamManager {
 public:
  static StatusOr<std::unique_ptr<StreamManager>> CreateStreamManager(
      const StreamManagerOnPremConfig& config) {
    auto manager_ptr = new OnPremStreamManagerImpl(config);
    AIS_RETURN_IF_ERROR(manager_ptr->Initialize());
    return std::unique_ptr<StreamManager>(manager_ptr);
  }

  // CreateStream creates the stream. Return status to indicate whether the
  // creation succeeded or failed.
  virtual StatusOr<Stream> CreateStream(const Stream& stream) override {
    grpc::ClientContext context;
    AIS_RETURN_IF_ERROR(FillGrpcClientContext(options_.rpc_options, &context));

    auto status = UpdateContext(context);
    if (!status.ok()) {
      LOG(ERROR) << status;
      return InternalError("Failed to update context.");
    }

    CreateStreamRequest request;
    CreateStreamResponse response;
    request.set_stream_name(stream.name());

    grpc::Status grpc_status =
        stub_->CreateStream(&context, request, &response);
    if (!grpc_status.ok()) {
      return UnknownError("Encountered error calling RPC CreateStream");
    }
    return stream;
  }

  // DeleteStream deletes the stream. Return status to indicate whether the
  // deletion succeeded or failed.
  virtual Status DeleteStream(const std::string& stream_name) override {
    grpc::ClientContext context;
    AIS_RETURN_IF_ERROR(FillGrpcClientContext(options_.rpc_options, &context));

    auto status = UpdateContext(context);
    if (!status.ok()) {
      LOG(ERROR) << status;
      return InternalError("Failed to update context.");
    }

    DeleteStreamRequest request;
    google::protobuf::Empty response;
    request.set_stream_name(stream_name);

    grpc::Status grpc_status =
        stub_->DeleteStream(&context, request, &response);
    if (!grpc_status.ok()) {
      LOG(ERROR) << grpc_status.error_message();
      return UnknownError("Encountered error calling RPC DeleteStream");
    }
    return OkStatus();
  }

  // ListStreams lists streams. Return the list of streams if the request
  // succeeds.
  virtual StatusOr<std::vector<Stream>> ListStreams() override {
    grpc::ClientContext context;
    AIS_RETURN_IF_ERROR(FillGrpcClientContext(options_.rpc_options, &context));

    auto status = UpdateContext(context);
    if (!status.ok()) {
      LOG(ERROR) << status;
      return InternalError("Failed to update context.");
    }

    ListStreamRequest request;
    ListStreamResponse response;
    grpc::Status grpc_status = stub_->ListStream(&context, request, &response);
    if (!grpc_status.ok()) {
      LOG(ERROR) << grpc_status.error_message();
      return UnknownError("Encountered error calling RPC ListStreams");
    }
    std::vector<Stream> streams;
    streams.reserve(response.stream_names_size());
    for (const auto& stream_name : response.stream_names()) {
      Stream stream;
      stream.set_name(stream_name);
      streams.push_back(stream);
    }
    return streams;
  }

 private:
  Status UpdateContext(grpc::ClientContext& context) {
    auto token_statusor = GetIdTokenWithDefaultServiceAccount();
    if (!token_statusor.ok()) {
      LOG(ERROR) << token_statusor.status();
      return InternalError("Failed to get token.");
    }
    context.AddMetadata(
        kAuthorization,
        absl::StrFormat(kTokenFormat, std::move(token_statusor).ValueOrDie()));
    return OkStatus();
  }

  Status Initialize() {
    auto grpc_channel = CreateGrpcChannel(options_);
    if (grpc_channel == nullptr) {
      return UnknownError("Failed to create a gRPC channel");
    }

    stub_ = ManagementServer::NewStub(grpc_channel);
    if (stub_ == nullptr) {
      return UnknownError("Failed to create a gRPC stub");
    }
    return OkStatus();
  }

  OnPremStreamManagerImpl(const StreamManagerOnPremConfig& config) {
    options_.target_address = config.target_address();

    // Setup ssl options.
    options_.ssl_options.ssl_domain_name = config.ssl_domain_name();
    options_.ssl_options.use_insecure_channel = config.use_insecure_channel();
    options_.ssl_options.ssl_root_cert_path = config.ssl_root_cert_path();

    // Setup rpc options.
    options_.rpc_options.wait_for_ready = config.wait_for_ready();
    options_.rpc_options.timeout = absl::Seconds(config.timeout().seconds()) +
                                   absl::Nanoseconds(config.timeout().nanos());
  }

  ConnectionOptions options_;
  std::unique_ptr<ManagementServer::Stub> stub_ = nullptr;
};

// Stream manager for managing streams in Google managed cluster.
class ManagedStreamManagerImpl : public StreamManager {
 public:
  static StatusOr<std::unique_ptr<StreamManager>> CreateStreamManager(
      const StreamManagerManagedConfig& config) {
    auto manager_ptr = new ManagedStreamManagerImpl(config);
    AIS_RETURN_IF_ERROR(manager_ptr->Initialize());
    return std::unique_ptr<StreamManager>(manager_ptr);
  }

  // CreateStream creates the stream. Return status to indicate whether the
  // creation succeeded or failed.
  virtual StatusOr<Stream> CreateStream(const Stream& stream) override {
    grpc::ClientContext context;
    ::google::partner::aistreams::v1alpha1::CreateStreamRequest request;
    ::google::longrunning::Operation operation;
    request.set_parent(parent_);
    request.set_stream_id(stream.name());
    // Needs to add metadata. GFE needs the metadata for routing request.
    context.AddMetadata(kGrpcMetadata, absl::StrFormat("parent=%s", parent_));

    grpc::Status grpc_status =
        stub_->CreateStream(&context, request, &operation);
    if (!grpc_status.ok()) {
      LOG(INFO) << grpc_status.error_message();
      return UnknownError("Encountered error calling RPC CreateStream");
    }

    LOG(INFO) << "Successfully sent request. Return long running operation: "
              << operation.name();
    return stream;
  }

  // DeleteStream deletes the stream. Return status to indicate whether the
  // deletion succeeded or failed.
  virtual Status DeleteStream(const std::string& stream_name) override {
    grpc::ClientContext context;
    ::google::partner::aistreams::v1alpha1::DeleteStreamRequest request;
    ::google::longrunning::Operation operation;
    request.set_name(stream_name);
    // Needs to add metadata. GFE needs the metadata for routing request.
    context.AddMetadata(kGrpcMetadata, absl::StrFormat("parent=%s", parent_));
    grpc::Status grpc_status =
        stub_->DeleteStream(&context, request, &operation);
    if (!grpc_status.ok()) {
      return UnknownError("Encountered error calling RPC DeleteStream");
    }

    LOG(INFO) << "Successfully sent request. Return long running operation: "
              << operation.name();
    return OkStatus();
  }

  // ListStreams lists streams. Return the list of streams if the request
  // succeeds.
  virtual StatusOr<std::vector<Stream>> ListStreams() override {
    grpc::ClientContext context;
    ::google::partner::aistreams::v1alpha1::ListStreamsRequest request;
    ::google::partner::aistreams::v1alpha1::ListStreamsResponse response;
    request.set_parent(parent_);

    // Needs to add metadata. GFE needs the metadata for routing request.
    context.AddMetadata(kGrpcMetadata, absl::StrFormat("parent=%s", parent_));
    grpc::Status grpc_status = stub_->ListStreams(&context, request, &response);
    if (!grpc_status.ok()) {
      LOG(ERROR) << grpc_status.error_message();
      return UnknownError("Encountered error calling RPC ListStreams");
    }

    std::vector<Stream> streams;
    streams.reserve(response.streams_size());
    for (const auto& s : response.streams()) {
      Stream stream;
      stream.set_name(s.name());
      streams.push_back(stream);
    }
    return streams;
  }

 private:
  Status Initialize() {
    auto channel = CreateGrpcChannel(options_);
    if (channel == nullptr) {
      return UnknownError("Failed to create a gRPC channel");
    }
    stub_ = AIStreams::NewStub(channel);
    if (stub_ == nullptr) {
      return UnknownError("Failed to create a gRPC stub");
    }
    return OkStatus();
  }

  ManagedStreamManagerImpl(const StreamManagerManagedConfig& config)
      : parent_(absl::StrFormat("projects/%s/locations/%s/clusters/%s",
                                config.project(), config.location(),
                                config.cluster())) {
    options_.target_address = config.target_address();
    options_.authenticate_with_google = true;
  }

  ConnectionOptions options_;
  std::unique_ptr<AIStreams::Stub> stub_ = nullptr;
  const std::string parent_;
};

class ClusterManagerImpl : public ClusterManager {
 public:
  static StatusOr<std::unique_ptr<ClusterManager>> CreateClusterManager(
      const ClusterManagerConfig& config) {
    auto manager_ptr = new ClusterManagerImpl(config);
    AIS_RETURN_IF_ERROR(manager_ptr->Initialize());
    return std::unique_ptr<ClusterManager>(manager_ptr);
  }

  // CreateCluster creates the cluster. Return status to indicate whether the
  // creation succeeded or failed.
  virtual StatusOr<Cluster> CreateCluster(const Cluster& cluster) {
    grpc::ClientContext context;
    ::google::partner::aistreams::v1alpha1::CreateClusterRequest request;
    ::google::longrunning::Operation operation;
    request.set_parent(parent_);
    request.set_cluster_id(cluster.name());
    // Needs to add metadata. GFE needs the metadata for routing request.
    context.AddMetadata(kGrpcMetadata, absl::StrFormat("parent=%s", parent_));

    grpc::Status grpc_status =
        stub_->CreateCluster(&context, request, &operation);
    if (!grpc_status.ok()) {
      LOG(ERROR) << grpc_status.error_message();
      return UnknownError("Encountered error calling RPC CreateCluster");
    }

    LOG(INFO) << "Successfully sent request. Return long running operation: "
              << operation.name();
    return cluster;
  }

  // DeleteCluster deletes the cluster. Returns status to indicate whether the
  // deletion succeeded of failed.
  virtual Status DeleteCluster(const std::string& cluster_name) {
    grpc::ClientContext context;
    ::google::partner::aistreams::v1alpha1::DeleteClusterRequest request;
    ::google::longrunning::Operation operation;
    request.set_name(cluster_name);
    // Needs to add metadata. GFE needs the metadata for routing request.
    context.AddMetadata(kGrpcMetadata, absl::StrFormat("parent=%s", parent_));
    grpc::Status grpc_status =
        stub_->DeleteCluster(&context, request, &operation);
    if (!grpc_status.ok()) {
      LOG(ERROR) << grpc_status.error_message();
      return UnknownError("Encountered error calling RPC DeleteCluster");
    }

    LOG(INFO) << "Successfully sent request. Return long running operation: "
              << operation.name();

    return OkStatus();
  }

  // ListClusters lists clusters. Returns the list of clusters if the request
  // succeeds.
  virtual StatusOr<std::vector<Cluster>> ListClusters() {
    grpc::ClientContext context;
    ::google::partner::aistreams::v1alpha1::ListClustersRequest request;
    ::google::partner::aistreams::v1alpha1::ListClustersResponse response;
    request.set_parent(parent_);

    // Needs to add metadata. GFE needs the metadata for routing request.
    context.AddMetadata(kGrpcMetadata, absl::StrFormat("parent=%s", parent_));
    grpc::Status grpc_status =
        stub_->ListClusters(&context, request, &response);
    if (!grpc_status.ok()) {
      LOG(ERROR) << grpc_status.error_message();
      return UnknownError("Encountered error calling RPC ListClusters");
    }

    std::vector<Cluster> clusters;
    clusters.reserve(response.clusters_size());
    for (const auto& c : response.clusters()) {
      Cluster cluster;
      cluster.set_name(c.name());
      auto service_endpoint = c.service_endpoint();
      ReplaceServiceEndpointPort(service_endpoint);
      cluster.set_service_endpoint(service_endpoint);
      cluster.set_certificate(c.certificate());
      clusters.push_back(cluster);
    }
    return clusters;
  }

 private:
  Status Initialize() {
    auto channel = CreateGrpcChannel(options_);
    if (channel == nullptr) {
      return UnknownError("Failed to create a gRPC channel");
    }
    stub_ = AIStreams::NewStub(channel);
    if (stub_ == nullptr) {
      return UnknownError("Failed to create a gRPC stub");
    }
    return OkStatus();
  }

  ClusterManagerImpl(const ClusterManagerConfig& config)
      : parent_(absl::StrFormat("projects/%s/locations/%s", config.project(),
                                config.location())) {
    options_.target_address = config.target_address();
    options_.authenticate_with_google = true;
  }

  ConnectionOptions options_;
  std::unique_ptr<AIStreams::Stub> stub_ = nullptr;
  const std::string parent_;
};

StatusOr<std::unique_ptr<StreamManager>>
StreamManagerFactory::CreateStreamManager(const StreamManagerConfig& config) {
  if (config.has_stream_manager_onprem_config()) {
    return OnPremStreamManagerImpl::CreateStreamManager(
        config.stream_manager_onprem_config());
  }

  if (config.has_stream_manager_managed_config()) {
    return ManagedStreamManagerImpl::CreateStreamManager(
        config.stream_manager_managed_config());
  }

  return InvalidArgumentError(
      "Input config is invalid. Either stream_manager_onprem_config or "
      "stream_manager_managed_config should be specified.");
}

StatusOr<std::unique_ptr<ClusterManager>>
ClusterManagerFactory::CreateClusterManager(
    const ClusterManagerConfig& config) {
  return ClusterManagerImpl::CreateClusterManager(config);
}

}  // namespace aistreams
