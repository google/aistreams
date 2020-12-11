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

#include "aistreams/base/wrappers/receivers.h"

#include <functional>
#include <thread>

#include "absl/strings/str_format.h"
#include "absl/time/time.h"
#include "aistreams/base/connection_options.h"
#include "aistreams/base/wrappers/receiver_queue.h"
#include "aistreams/port/canonical_errors.h"
#include "aistreams/port/status.h"
#include "aistreams/port/status_macros.h"
#include "aistreams/port/statusor.h"

namespace aistreams {

namespace {
constexpr int kDefaultTryPushTimeoutSeconds = 5;
constexpr int kDefaultBufferCapacity = 300;
}  // namespace

Status MakePacketReceiverQueue(const ReceiverOptions& options,
                               ReceiverQueue<Packet>* receiver_queue) {
  // Create the shared producer/consumer queue.
  //
  // This share shall be transferred to the background producer.
  int capacity = options.buffer_capacity;
  if (capacity <= 0) {
    capacity = kDefaultBufferCapacity;
  }
  auto packet_queue = std::make_shared<ProducerConsumerQueue<Packet>>(capacity);

  // Create a receiver queue.
  //
  // Give it one share of the packet queue.
  // This will be transferred to the caller.
  *receiver_queue = ReceiverQueue<Packet>(packet_queue);

  // Create a PacketReceiver.
  PacketReceiver::Options packet_receiver_options;
  packet_receiver_options.connection_options = options.connection_options;
  packet_receiver_options.stream_name = options.stream_name;
  packet_receiver_options.receiver_name = options.receiver_name;
  packet_receiver_options.offset_options = options.offset_options;
  packet_receiver_options.receiver_mode = options.receiver_mode;
  auto packet_receiver_statusor =
      PacketReceiver::Create(packet_receiver_options);
  if (!packet_receiver_statusor.ok()) {
    LOG(ERROR) << packet_receiver_statusor.status();
    return UnknownError("Failed to create a PacketReceiver");
  }
  auto packet_receiver = std::move(packet_receiver_statusor).ValueOrDie();

  // Run the packet receiver in the background.
  //
  // Transfer the queue and its producer share.
  std::thread packet_receiver_worker(
      [packet_queue = std::move(packet_queue),
       packet_receiver = std::move(packet_receiver)]() {
        Status s;
        std::unique_ptr<Packet> p;
        while (packet_queue.use_count() > 1) {
          if (p == nullptr) {
            p = std::make_unique<Packet>();
            s = packet_receiver->Receive(p.get());
            if (!s.ok()) {
              packet_queue->Emplace(
                  MakeEosPacket(
                      absl::StrFormat(
                          "Could not receive a packet from the server: %s",
                          s.message()))
                      .ValueOrDie());
              break;
            }
          }
          if (!packet_queue->TryPush(
                  p, absl::Seconds(kDefaultTryPushTimeoutSeconds))) {
            LOG(WARNING) << "The shared producer consumer queue is full";
          }
        }
        return;
      });
  packet_receiver_worker.detach();
  return OkStatus();
}

Status ReceivePackets(const ReceiverOptions& options, absl::Duration timeout,
                      const std::function<Status(Packet)>& callback) {
  auto packet_receiver_queue = std::make_unique<ReceiverQueue<Packet>>();
  auto status = MakePacketReceiverQueue(options, packet_receiver_queue.get());
  if (!status.ok()) {
    LOG(ERROR) << status;
    return UnknownError("Failed to create a packet receiver queue");
  }

  Packet p;
  while (true) {
    if (!packet_receiver_queue->TryPop(p, timeout)) {
      auto eos_packet_statusor =
          MakeEosPacket("Timed out waiting for a new packet");
      if (!eos_packet_statusor.ok()) {
        LOG(ERROR) << eos_packet_statusor.status();
        return InternalError("Failed to create an EOS packet");
      }
      auto status = callback(std::move(eos_packet_statusor).ValueOrDie());
      if (IsCancelled(status)) {
        break;
      }
      if (!status.ok()) {
        LOG(ERROR) << "Consumer callback returned an error handling EOS: "
                   << status.message();
      }
      continue;
    }

    auto status = callback(std::move(p));
    if (IsCancelled(status)) {
      break;
    } else if (!status.ok()) {
      LOG(ERROR) << "Consumer callback returned error: " << status.message();
    }
  }
  return OkStatus();
}

}  // namespace aistreams
