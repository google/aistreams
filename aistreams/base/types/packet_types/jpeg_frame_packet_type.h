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

#ifndef AISTREAMS_BASE_TYPES_PACKET_TYPES_JPEG_FRAME_PACKET_TYPE_H_
#define AISTREAMS_BASE_TYPES_PACKET_TYPES_JPEG_FRAME_PACKET_TYPE_H_

#include "aistreams/base/types/jpeg_frame.h"
#include "aistreams/base/types/packet_types/packet_type_traits.h"
#include "aistreams/port/status.h"
#include "aistreams/proto/packet.pb.h"

namespace aistreams {

// Specialization to map JpegFrames to Packets of type PACKET_TYPE_JPEG.
template <>
struct PacketTypeTraits<JpegFrame> {
  using value_type = JpegFrame;
  constexpr static PacketTypeId packet_type_id() { return PACKET_TYPE_JPEG; }

  constexpr static const char* packet_type_name() { return "JpegFrame"; }

  static Status packet_type_descriptor(const JpegFrame& jpeg_frame,
                                       google::protobuf::Any* any) {
    return OkStatus();
  }
};

// Pack the Packet's payload with copy semantics.
Status PackPayload(const JpegFrame&, Packet*);

// Pack the Packet's payload with move semantics.
Status PackPayload(JpegFrame&&, Packet*);

// Unpack the Packet's payload with copy semantics.
Status UnpackPayload(const Packet& p, JpegFrame*);

// Unpack the Packet's payload with move semantics.
Status UnpackPayload(Packet&& p, JpegFrame*);

}  // namespace aistreams

#endif  // AISTREAMS_BASE_TYPES_PACKET_TYPES_JPEG_FRAME_PACKET_TYPE_H_
