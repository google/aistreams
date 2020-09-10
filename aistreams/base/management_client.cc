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

#include "aistreams/base/util/grpc_helpers.h"
#include "aistreams/port/canonical_errors.h"
#include "aistreams/port/status.h"
#include "aistreams/port/status_macros.h"
#include "aistreams/proto/management.grpc.pb.h"
#include "aistreams/proto/management.pb.h"

namespace aistreams {

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

    CreateStreamRequest request;
    CreateStreamResponse response;
    request.set_stream_name(stream.name());

    grpc::Status grpc_status =
        stub_->CreateStream(&context, request, &response);
    if (!grpc_status.ok()) {
      LOG(ERROR) << grpc_status.error_message();
      return UnknownError("Encountered error calling RPC CreateStream");
    }
    return OkStatus();
  }

  // DeleteStream deletes the stream. Return status to indicate whether the
  // deletion succeeded or failed.
  virtual Status DeleteStream(const std::string& stream_name) override {
    grpc::ClientContext context;
    AIS_RETURN_IF_ERROR(FillGrpcClientContext(options_.rpc_options, &context));

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
      const StreamManagerManagedConfig& config);

  // CreateStream creates the stream. Return status to indicate whether the
  // creation succeeded or failed.
  virtual StatusOr<Stream> CreateStream(const Stream& stream) override;

  // DeleteStream deletes the stream. Return status to indicate whether the
  // deletion succeeded or failed.
  virtual Status DeleteStream(const std::string& stream_name) override;

  // ListStreams lists streams. Return the list of streams if the request
  // succeeds.
  virtual StatusOr<std::vector<Stream>> ListStreams() override;

 private:
  ManagedStreamManagerImpl(const StreamManagerManagedConfig& config);
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
