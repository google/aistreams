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

#include "aistreams/base/wrappers/managers.h"

#include "aistreams/port/canonical_errors.h"

namespace aistreams {

Status MakeStreamManager(const StreamManagerConfig& config,
                         std::unique_ptr<StreamManager>* stream_manager) {
  auto stream_manager_statusor =
      StreamManagerFactory::CreateStreamManager(config);
  if (!stream_manager_statusor.ok()) {
    LOG(ERROR) << stream_manager_statusor.status();
    return UnknownError("Failed to create a StreamManager");
  }
  *stream_manager = std::move(stream_manager_statusor).ValueOrDie();
  return OkStatus();
}

}  // namespace aistreams