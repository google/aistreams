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

#ifndef AISTREAMS_BASE_MANAGEMENT_CLIENT_H_
#define AISTREAMS_BASE_MANAGEMENT_CLIENT_H_

#include <string>

#include "aistreams/base/connection_options.h"
#include "aistreams/port/status.h"
#include "aistreams/port/statusor.h"
#include "aistreams/proto/management.grpc.pb.h"

namespace aistreams {
namespace base {

// Management client for creating/deleting/listing streams.
//
// TODO(yxyan): add interface for ManagementClient and add mock for
// the client.
class ManagementClient {
 public:
  static StatusOr<std::unique_ptr<ManagementClient>> Create(
      const ConnectionOptions& options);

  Status CreateStream(const std::string& stream_name);

  Status DeleteStream(const std::string& stream_name);

 private:
  ManagementClient(const ConnectionOptions& options);
  Status Initialize();

  const ConnectionOptions options_;
  std::unique_ptr<ManagementServer::Stub> stub_ = nullptr;
};

}  // namespace base
}  // namespace aistreams

#endif  // AISTREAMS_BASE_MANAGEMENT_CLIENT_H_
