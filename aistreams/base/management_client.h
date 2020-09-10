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

#include "aistreams/base/connection_options.h"
#include "aistreams/port/canonical_errors.h"
#include "aistreams/port/status.h"
#include "aistreams/port/statusor.h"
#include "aistreams/proto/management.grpc.pb.h"
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

// Stream manager for managing streams in on-prem cluster.
class OnPremStreamManagerImpl : public StreamManager {
 public:
  static StatusOr<std::unique_ptr<StreamManager>> CreateStreamManager(
      const StreamManagerOnPremConfig& config);

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
  OnPremStreamManagerImpl(const StreamManagerOnPremConfig& config);
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

// Factory for creating stream manager.
class StreamManagerFactory {
 public:
  // CreateStreamManager creates the stream manager given the
  static StatusOr<std::unique_ptr<StreamManager>> CreateStreamManager(
      const StreamManagerConfig& config) {
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
};
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

}  // namespace aistreams

#endif  // AISTREAMS_BASE_MANAGEMENT_CLIENT_H_
