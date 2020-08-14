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

#include "aistreams/base/stream_channel.h"

#include <utility>

#include "aistreams/base/util/grpc_helpers.h"
#include "aistreams/port/canonical_errors.h"
#include "aistreams/port/status.h"
#include "aistreams/port/status_macros.h"

namespace aistreams {
namespace base {

namespace {
constexpr char kStreamMetadataKeyName[] = "stream";
}

StreamChannel::StreamChannel(const Options& options) : options_(options) {}

Status StreamChannel::Initialize() {
  // Create a grpc channel.
  ConnectionOptions connection_options;
  connection_options.target_address = options_.target_address;
  connection_options.ssl_options = options_.ssl_options;
  grpc_channel_ = CreateGrpcChannel(connection_options);
  if (grpc_channel_ == nullptr) {
    return UnknownError("Failed to create a gRPC channel");
  }
  return OkStatus();
}

StatusOr<std::unique_ptr<grpc::ClientContext>>
StreamChannel::MakeClientContext() const {
  auto ctx = std::make_unique<grpc::ClientContext>();
  // Note: When the stream_name is not empty, we assume it is to be included
  // in the grpc message's metadata. This is ignored if the target_address is to
  // the stream server itself. If the target_address is to an ingress, then this
  // field is used to redirect.
  if (!options_.stream_name.empty()) {
    ctx->AddMetadata(kStreamMetadataKeyName, options_.stream_name);
  }

  // If set, if an RPC is made when a channel's connectivity state is
  // TRANSIENT_FAILURE or CONNECTING, the call will not "fail fast", and the
  // channel will wait until the channel is READY before making the call.
  if (options_.wait_for_ready) {
    ctx->set_wait_for_ready(true);
  }
  return ctx;
}

StatusOr<std::unique_ptr<StreamChannel>> StreamChannel::Create(
    const Options& options) {
  auto stream_channel = std::make_unique<StreamChannel>(options);
  AIS_RETURN_IF_ERROR(stream_channel->Initialize());
  return stream_channel;
}

}  // namespace base
}  // namespace aistreams
