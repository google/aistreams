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
#include "aistreams/proto/management.pb.h"

namespace aistreams {
namespace base {

StatusOr<std::unique_ptr<ManagementClient>> ManagementClient::Create(
    const ConnectionOptions& options) {
  std::unique_ptr<ManagementClient> client(new ManagementClient(options));
  AIS_RETURN_IF_ERROR(client->Initialize());
  return client;
}

ManagementClient::ManagementClient(const ConnectionOptions& options)
    : options_(options) {}

Status ManagementClient::Initialize() {
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

Status ManagementClient::CreateStream(const std::string& stream_name) {
  grpc::ClientContext context;
  AIS_RETURN_IF_ERROR(FillGrpcClientContext(options_.rpc_options, &context));

  CreateStreamRequest request;
  CreateStreamResponse response;
  request.set_stream_name(stream_name);

  grpc::Status grpc_status = stub_->CreateStream(&context, request, &response);
  if (!grpc_status.ok()) {
    LOG(ERROR) << grpc_status.error_message();
    return UnknownError("Encountered error calling RPC CreateStream");
  }
  return OkStatus();
}

Status ManagementClient::DeleteStream(const std::string& stream_name) {
  grpc::ClientContext context;
  AIS_RETURN_IF_ERROR(FillGrpcClientContext(options_.rpc_options, &context));

  DeleteStreamRequest request;
  google::protobuf::Empty response;
  request.set_stream_name(stream_name);

  grpc::Status grpc_status = stub_->DeleteStream(&context, request, &response);
  if (!grpc_status.ok()) {
    LOG(ERROR) << grpc_status.error_message();
    return UnknownError("Encountered error calling RPC DeleteStream");
  }
  return OkStatus();
}

}  // namespace base
}  // namespace aistreams
