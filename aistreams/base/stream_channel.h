/*
 * Copyright 2020 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef AISTREAMS_BASE_STREAM_CHANNEL_H_
#define AISTREAMS_BASE_STREAM_CHANNEL_H_

#include <memory>

#include "aistreams/base/connection_options.h"
#include "aistreams/port/grpcpp.h"
#include "aistreams/port/status.h"
#include "aistreams/port/statusor.h"

namespace aistreams {

// A class that establishes a gRPC client channel and configures an RPC client
// context suitable for interacting with the stream server.
class StreamChannel {
 public:
  // Options to configure the connection channel.
  struct Options {
    // Options to configure the RPC connection.
    ConnectionOptions connection_options;

    // The stream name to connect to.
    //
    // Note: This is needed if `ConnectionOptions.target_address` is to the
    // ingress. You can leave this empty if you are directly connecting to the
    // stream server.
    std::string stream_name;
  };

  // Creates and initializes an instance that is ready for use.
  static StatusOr<std::unique_ptr<StreamChannel>> Create(const Options&);

  // Get a shared pointer to the established client channel.
  std::shared_ptr<grpc::Channel> GetChannel() const { return grpc_channel_; }

  // Create a configured RPC client context.
  StatusOr<std::unique_ptr<grpc::ClientContext>> MakeClientContext() const;

  // Use Create instead of the bare constructors.
  StreamChannel(const Options&);

 private:
  Options options_;
  std::shared_ptr<grpc::Channel> grpc_channel_ = nullptr;

  Status Initialize();
};

}  // namespace aistreams

#endif  // AISTREAMS_BASE_STREAM_CHANNEL_H_
