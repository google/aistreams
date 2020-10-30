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

#include "aistreams/base/wrappers/senders.h"

#include <functional>
#include <thread>

#include "aistreams/base/packet_sender.h"
#include "aistreams/port/canonical_errors.h"
#include "aistreams/port/status.h"

namespace aistreams {

Status MakePacketSender(const SenderOptions& options,
                        std::unique_ptr<PacketSender>* sender) {
  // Create a PacketSender that uses uni-directional streaming gRPC.
  PacketSender::Options packet_sender_options;
  packet_sender_options.connection_options = options.connection_options;
  packet_sender_options.stream_name = options.stream_name;
  packet_sender_options.trace_probability = options.trace_probability;
  auto packet_sender_statusor = PacketSender::Create(packet_sender_options);
  if (!packet_sender_statusor.ok()) {
    LOG(ERROR) << packet_sender_statusor.status();
    return UnknownError("Failed to create a PacketSender");
  }
  *sender = std::move(packet_sender_statusor).ValueOrDie();
  return OkStatus();
}

}  // namespace aistreams
