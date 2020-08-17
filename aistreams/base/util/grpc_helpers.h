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

#ifndef AISTREAMS_BASE_UTIL_GRPC_HELPERS_H_
#define AISTREAMS_BASE_UTIL_GRPC_HELPERS_H_

#include <memory>
#include <string>

#include "absl/strings/string_view.h"
#include "aistreams/base/connection_options.h"
#include "aistreams/port/grpcpp.h"
#include "aistreams/port/status.h"

namespace aistreams {

// Helper to create a gRPC channel; optionally enables SSL.
//
// Returns `nullptr` on error.
std::shared_ptr<grpc::Channel> CreateGrpcChannel(
    const ConnectionOptions &options);

// Helper to fill a grpc::ClientContext given a RpcOption.
Status FillGrpcClientContext(const RpcOptions &options,
                             grpc::ClientContext *ctx);

}  // namespace aistreams

#endif  // AISTREAMS_BASE_UTIL_GRPC_HELPERS_H_
