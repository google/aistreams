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

#include "absl/strings/str_format.h"
#include "aistreams/base/util/auth_helpers.h"
#include "aistreams/base/util/grpc_helpers.h"
#include "aistreams/port/canonical_errors.h"
#include "aistreams/port/status.h"
#include "aistreams/port/status_macros.h"
#include "aistreams/util/constants.h"

namespace aistreams {

using ::aistreams::constants::kStreamMetadataKeyName;

StreamChannel::StreamChannel(const Options& options) : options_(options) {}

Status StreamChannel::Initialize() {
  // Establish a grpc channel.
  grpc_channel_ = CreateGrpcChannel(options_.connection_options);
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

  // Active only for the managed service. This fetches the JWT token and uses it
  // to authenticate against the k8s Ingress.
  if (options_.connection_options.authenticate_with_google) {
    auto token_statusor = GetIdTokenWithDefaultServiceAccount();
    if (!token_statusor.ok()) {
      LOG(WARNING) << "failed to get ID token" << token_statusor.status();
    } else {
      ctx->AddMetadata(
          "authorization",
          absl::StrFormat("Bearer %s", std::move(token_statusor).ValueOrDie()));
    }
  }

  // Configure the RPC options.
  AIS_RETURN_IF_ERROR(FillGrpcClientContext(
      options_.connection_options.rpc_options, ctx.get()));

  return ctx;
}

StatusOr<std::unique_ptr<StreamChannel>> StreamChannel::Create(
    const Options& options) {
  auto stream_channel = std::make_unique<StreamChannel>(options);
  AIS_RETURN_IF_ERROR(stream_channel->Initialize());
  return stream_channel;
}

}  // namespace aistreams
