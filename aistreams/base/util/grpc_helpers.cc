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

#include "aistreams/base/util/grpc_helpers.h"

#include "absl/time/clock.h"
#include "absl/time/time.h"
#include "aistreams/port/canonical_errors.h"
#include "aistreams/port/logging.h"
#include "aistreams/port/status.h"
#include "aistreams/port/status_macros.h"
#include "aistreams/port/statusor.h"
#include "aistreams/util/file_helpers.h"

namespace aistreams {

namespace {

void SetCommonChannelArgs(grpc::ChannelArguments &channel_args) {
  channel_args.SetMaxReceiveMessageSize(-1);
  channel_args.SetMaxSendMessageSize(-1);
  return;
}

std::shared_ptr<grpc::Channel> CreateInsecureGrpcChannel(
    const std::string &target_address) {
  grpc::ChannelArguments channel_args;
  SetCommonChannelArgs(channel_args);
  std::shared_ptr<grpc::Channel> channel = grpc::CreateCustomChannel(
      target_address, grpc::InsecureChannelCredentials(), channel_args);
  return channel;
}

std::shared_ptr<grpc::Channel> CreateSecureGrpcChannel(
    const std::string &target_address, const std::string &ssl_domain_name,
    const std::string &ssl_root_cert_path) {
  // Get SSL certificates.
  grpc::SslCredentialsOptions ssl_options;
  auto status =
      file::GetContents(ssl_root_cert_path, &ssl_options.pem_root_certs);
  if (!status.ok()) {
    LOG(ERROR) << "Failed to GetContents of the SSL cert file: " << status;
    return nullptr;
  }

  // Set channel args.
  std::shared_ptr<grpc::ChannelCredentials> channel_credentials =
      grpc::SslCredentials(ssl_options);
  grpc::ChannelArguments channel_args;
  SetCommonChannelArgs(channel_args);
  channel_args.SetSslTargetNameOverride(ssl_domain_name);

  // Create the channel.
  std::shared_ptr<grpc::Channel> channel = grpc::CreateCustomChannel(
      target_address, channel_credentials, channel_args);
  return channel;
}

}  // namespace

std::shared_ptr<grpc::Channel> CreateGrpcChannel(
    const ConnectionOptions &options) {
  const SslOptions &ssl_options = options.ssl_options;
  if (ssl_options.use_insecure_channel) {
    return CreateInsecureGrpcChannel(options.target_address);
  } else {
    return CreateSecureGrpcChannel(options.target_address,
                                   ssl_options.ssl_domain_name,
                                   ssl_options.ssl_root_cert_path);
  }
}

Status FillGrpcClientContext(const RpcOptions &options,
                             grpc::ClientContext *ctx) {
  ctx->set_wait_for_ready(options.wait_for_ready);
  if (options.timeout > absl::ZeroDuration() &&
      options.timeout < absl::InfiniteDuration()) {
    ctx->set_deadline(ToChronoTime(absl::Now() + options.timeout));
  }
  return OkStatus();
}

}  // namespace aistreams
