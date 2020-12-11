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

#ifndef AISTREAMS_BASE_WRAPPERS_RECEIVERS_H_
#define AISTREAMS_BASE_WRAPPERS_RECEIVERS_H_

#include <functional>

#include "aistreams/base/connection_options.h"
#include "aistreams/base/offset_options.h"
#include "aistreams/base/packet.h"
#include "aistreams/base/packet_receiver.h"
#include "aistreams/base/wrappers/receiver_queue.h"
#include "aistreams/port/status.h"
#include "aistreams/port/statusor.h"

namespace aistreams {

// Options to configure a receiver.
struct ReceiverOptions {
  // Options to connect to the the service.
  ConnectionOptions connection_options;

  // Options to specify the offset to start receiving.
  OffsetOptions offset_options;

  // The name of the stream to connect to.
  std::string stream_name;

  // A name that identifies, you, the receiver.
  //
  // You may leave this empty; in this case, you will get a random assignment.
  std::string receiver_name;

  // The capacity of the queue used to buffer packets.
  //
  // Non-positive values will resolve to a pre-configured default.
  int buffer_capacity = 0;

  // The receiver mode.
  ReceiverMode receiver_mode = ReceiverMode::StreamingReceive;
};

// Create a ReceiverQueue containing packets arriving from the server.
//
// The Packet influx will be paused if the receiver queue becomes full.
// The TryPop method in ReceiverQueue will timeout if the queue stays empty.
//
// The Packets arriving in the receiver queue have the following properties:
// 1. (Ordered) Packets are queued in the same order as they were in the stream.
// 2. (Gapless) If two Packets in the stream were queued, then so did all
//    Packets that were in between them.
// 3. Packets that arrive either contain data or represent EOS. It is your
//    responsibility to check for EOS (e.g. using IsEos).
Status MakePacketReceiverQueue(const ReceiverOptions& options,
                               ReceiverQueue<Packet>* receiver_queue);

// Run `callback` on every Packet that is received from the stream. This
// function returns when the `callback` returns a kCancelled Status.
//
// If no new Packets arrive from the server in `timeout`, then `callback` will
// be given an EOS packet.
//
// The Packets are arriving at the `callback` satsify the same properties
// outlined in MakePacketReceiverQueue above.
Status ReceivePackets(const ReceiverOptions&, absl::Duration timeout,
                      const std::function<Status(Packet)>& callback);

}  // namespace aistreams

#endif  // AISTREAMS_BASE_WRAPPERS_RECEIVERS_H_
