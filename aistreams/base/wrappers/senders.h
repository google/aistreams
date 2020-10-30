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

#ifndef AISTREAMS_BASE_WRAPPERS_SENDERS_H_
#define AISTREAMS_BASE_WRAPPERS_SENDERS_H_

#include <functional>

#include "aistreams/base/connection_options.h"
#include "aistreams/base/packet.h"
#include "aistreams/base/packet_sender.h"
#include "aistreams/port/status.h"
#include "aistreams/port/statusor.h"

namespace aistreams {

// Options to configure a sender.
struct SenderOptions {
  // Options to connect to the the service.
  ConnectionOptions connection_options;

  // The name of the stream to connect to.
  std::string stream_name;

  // The probability to start a trace for each packet sent.
  double trace_probability = 0;
};

// Create a packet sender.
//
// The PacketSender created with this function is configured such that
// PacketSender::Send(const Packet&) fails only when the underlying connection
// is broken. If you wish to continue sending, you should arrange for
// re-connections and backoffs; e.g. running MakePacketSender again.
//
// Normally, you should send a sequence of data Packets of the same type. When
// you are done, make sure to send an EOS packet before destroying the
// SenderQueue. You can create an EOS packet, e.g. using MakeEosPacket.
//
// TODO: At the moment PacketSender::Send() will not guarantee that the server
// has actually accepted the packet. There are several ways forward, but this
// is planned to arrive in beta/GA.
Status MakePacketSender(const SenderOptions& options,
                        std::unique_ptr<PacketSender>* sender);

}  // namespace aistreams

#endif  // AISTREAMS_BASE_WRAPPERS_SENDERS_H_
