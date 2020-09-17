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
using ::google::longrunning::Operation;
using ::google::longrunning::Operations;
using ::google::partner::aistreams::v1alpha1::AIStreams;
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
    grpc::Status grpc_status =
        stub_->CreateStream(&context, request, &operation);
    if (!grpc_status.ok()) {
      return UnknownError("Encountered error calling RPC CreateStream");
    }

    auto operation_statusor = WaitOperation(operation);
    if (!operation_statusor.ok()) {
      LOG(ERROR) << operation_statusor.status();
      return UnknownError("Excountered error calling WaitOperation");
    }
    operation = std::move(operation_statusor).ValueOrDie();
    ::google::partner::aistreams::v1alpha1::Stream s;
    if (!operation.response().UnpackTo(&s)) {
      return UnknownError(
          "Encountered error while unpack response to stream message.");
    }
    Stream result;
    result.set_name(s.name());
    return result;
  }

  // DeleteStream deletes the stream. Return status to indicate whether the
  // deletion succeeded or failed.
  virtual Status DeleteStream(const std::string& stream_name) override {
    grpc::ClientContext context;
    ::google::partner::aistreams::v1alpha1::DeleteStreamRequest request;
    ::google::longrunning::Operation operation;
    request.set_name(stream_name);
    grpc::Status grpc_status =
        stub_->DeleteStream(&context, request, &operation);
    if (!grpc_status.ok()) {
      return UnknownError("Encountered error calling RPC DeleteStream");
    }

    auto operation_statusor = WaitOperation(operation);
    if (!operation_statusor.ok()) {
      LOG(ERROR) << operation_statusor.status();
      return UnknownError("Excountered error calling WaitOperation");
    }
    return OkStatus();
  }

  // ListStreams lists streams. Return the list of streams if the request
  // succeeds.
  virtual StatusOr<std::vector<Stream>> ListStreams() override {
    grpc::ClientContext context;
    ::google::partner::aistreams::v1alpha1::ListStreamsRequest request;
    ::google::partner::aistreams::v1alpha1::ListStreamsResponse response;
    grpc::Status grpc_status = stub_->ListStreams(&context, request, &response);
    if (!grpc_status.ok()) {
      return UnknownError("Encountered error calling RPC CreateStream");
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

  StatusOr<Operation> WaitOperation(const Operation& operation) {
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
    request.set_name(operation.name());
    grpc::ClientContext context;
    auto grpc_status = stub->WaitOperation(&context, request, &response);
    if (!grpc_status.ok()) {
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

}  // namespace aistreams
