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

#ifndef AISTREAMS_BASE_UTIL_PACKET_UTILS_H_
#define AISTREAMS_BASE_UTIL_PACKET_UTILS_H_

#include <vector>

#include "absl/strings/str_format.h"
#include "aistreams/port/canonical_errors.h"
#include "aistreams/port/status.h"
#include "aistreams/port/statusor.h"
#include "aistreams/proto/packet.pb.h"
#include "aistreams/proto/types/control_signal.pb.h"
#include "google/protobuf/message.h"

namespace aistreams {

// ------------------------------------------------------------------
// Packet type utilities.

// Get the PacketTypeId.
PacketTypeId GetPacketTypeId(const Packet&);

// Check if the given Packet is a control signal packet.
bool IsControlSignal(const Packet&);

// Get the ControlSignalTypeId of the given packet.
//
// This may not succeed for any number of reason (e.g. p is not a control
// signal). In these cases, the status will tell the reason.
StatusOr<ControlSignalTypeId> GetControlSignalTypeId(const Packet& p);

// ------------------------------------------------------------------
// EOS utilities.

// Check if the given Packet is an EOS control signal packet.
bool IsEos(const Packet&);

// Check if the given Packet is an EOS control signal packet.
//
// If not return false with no side effects. Otherwise, returns true and if
// `reason` is not nullptr, it will set its pointee to indicate why the Eos was
// sent.
bool IsEos(const Packet&, std::string* reason);

// ------------------------------------------------------------------
// Packet addenda ("metadata") utilities.

// Inserts a string addendum with key `key` and value `value` into packet `p`.
Status InsertStringAddendum(const std::string& key, const std::string& value,
                            Packet* p);

// Inserts a protobuf addendum with key `key` and value `value` into packet `p`.
Status InsertProtoAddendum(const std::string& key,
                           const google::protobuf::Message& value, Packet* p);

// Deletes an addendum with key `key`.
Status DeleteAddendum(const std::string& key, Packet*);

// Get a copy of the string value from a string addendum with key `key`.
Status GetStringAddendum(const Packet& p, const std::string& key,
                         std::string* s);

// Get a copy of the protobuf value from a protobuf addendum with key `key`.
Status GetProtoAddendum(const Packet& p, const std::string& key,
                        google::protobuf::Message* m);

// ------------------------------------------------------------------
// Packet flags utilities.
//

// Returns true if and only if the given packet is a key frame.
//
// A key frame is any packet whose payload can be decoded independently of any
// other packets.
bool IsKeyFrame(const Packet&);

// (experimental) Returns true if and only if the given packet is a frame head.
//
// A frame head is a packet that is the first packet in a sequence of packets
// that forms a single coded picture.
bool IsFrameHead(const Packet&);

}  // namespace aistreams

#endif  // AISTREAMS_BASE_UTIL_PACKET_UTILS_H_
