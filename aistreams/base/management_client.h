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

#include "aistreams/port/status.h"
#include "aistreams/port/statusor.h"
#include "aistreams/proto/management.pb.h"

namespace aistreams {

// Manages clusters for hosting aistreams.
class ClusterManager {
 public:
  virtual ~ClusterManager() = default;
};

// Stream manager for managing aistreams.
class StreamManager {
 public:
  virtual ~StreamManager() = default;

  // CreateStream creates the stream. Return status to indicate whether the
  // creation succeeded or failed.
  virtual StatusOr<Stream> CreateStream(const Stream& stream) = 0;

  // DeleteStream deletes the stream. Return status to indicate whether the
  // deletion succeeded or failed.
  virtual Status DeleteStream(const std::string& stream_name) = 0;

  // ListStreams lists streams. Return the list of streams if the request
  // succeeds.
  virtual StatusOr<std::vector<Stream>> ListStreams() = 0;
};

// Factory for creating stream manager.
class StreamManagerFactory {
 public:
  // CreateStreamManager creates the stream manager given the
  static StatusOr<std::unique_ptr<StreamManager>> CreateStreamManager(
      const StreamManagerConfig& config);
};

}  // namespace aistreams

#endif  // AISTREAMS_BASE_MANAGEMENT_CLIENT_H_
