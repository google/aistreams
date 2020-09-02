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

#include "aistreams/base/util/grpc_helpers.h"

#include "aistreams/port/gtest.h"

namespace aistreams {

namespace {

TEST(GrpcHelpersTest, FillGrpcClientContext) {
  RpcOptions rpc_options;
  {
    grpc::ClientContext client_context;
    EXPECT_TRUE(FillGrpcClientContext(rpc_options, &client_context).ok());
    EXPECT_EQ(std::numeric_limits<int64_t>::max(),
              client_context.raw_deadline().tv_sec);
  }

  {
    grpc::ClientContext client_context;
    rpc_options.timeout = absl::Minutes(1);
    EXPECT_TRUE(FillGrpcClientContext(rpc_options, &client_context).ok());
    EXPECT_GE(std::numeric_limits<int64_t>::max(),
              client_context.raw_deadline().tv_sec);
  }
}

}  // namespace

}  // namespace aistreams
