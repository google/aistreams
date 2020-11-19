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

#ifndef AISTREAMS_BASE_PACKET_FLAGS_H_
#define AISTREAMS_BASE_PACKET_FLAGS_H_

#include "aistreams/port/status.h"
#include "aistreams/proto/packet.pb.h"

namespace aistreams {

// PacketFlags is a bitset of boolean attributes:
//
// You should use methods in packet_utils.h to access these flag values instead
// of directly using the methods here. These are mainly for library developers
// and type extenders. Please contact us if you fall into this category.
//
// + kEmpty: This is the zero value.
//
// + kIsFrameHead: This is set if this packet is the first packet in a sequence
//                 of packets that represent a single coded picture/data.
//
//                 For videos, this is codec/protocol dependent;
//                 e.g. it is set for h264 when packets enclose an access unit.
//
//                 This bit is set for all standard messages like images,
//                 protobufs, and strings.
//
// + kIsKeyFrame: This is set if this packet is a keyframe.
//
//                We define keyframes here as any packet whose payload can be
//                decoded correctly independent of other packets.
//                e.g. for videos, this is set for "I" frames in some codecs.
//
//                This bit is set for all standard messages like images,
//                protobufs, and strings.
enum class PacketFlags : int {
  kEmpty = 0x0,
  kIsFrameHead = 0x1 << 0,
  kIsKeyFrame = 0x1 << 1,
};

// Zero out all flag bits.
void ClearPacketFlags(Packet*);

// Set the bits indicated in `flags`.
void SetPacketFlags(PacketFlags flags, Packet*);

// Unset the bits indicated in `flags`.
void UnsetPacketFlags(PacketFlags flags, Packet*);

// Returns true if the indicated `flags` are set.
bool IsPacketFlagsSet(PacketFlags flags, const Packet&);

// Restores the packet flags to the defaults.
// The default value is selected based on the packet type.
Status RestoreDefaultPacketFlags(Packet*);

inline constexpr PacketFlags operator|(PacketFlags x, PacketFlags y) {
  return static_cast<PacketFlags>(static_cast<int>(x) | static_cast<int>(y));
}

}  // namespace aistreams

#endif  // AISTREAMS_BASE_PACKET_FLAGS_H_
