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

#ifndef AISTREAMS_BASE_TYPES_PACKET_TYPES_GSTREAMER_BUFFER_PACKET_TYPE_H_
#define AISTREAMS_BASE_TYPES_PACKET_TYPES_GSTREAMER_BUFFER_PACKET_TYPE_H_

#include "aistreams/base/types/gstreamer_buffer.h"
#include "aistreams/base/types/packet_types/packet_type_traits.h"
#include "aistreams/port/canonical_errors.h"
#include "aistreams/port/status.h"
#include "aistreams/proto/packet.pb.h"
#include "aistreams/proto/types/gstreamer_buffer_packet_type_descriptor.pb.h"

namespace aistreams {

// Specialization to map GstreamerBuffers to Packets of type
// PACKET_TYPE_GSTREAMER_BUFFER.
template <>
struct PacketTypeTraits<GstreamerBuffer> {
  using value_type = GstreamerBuffer;
  constexpr static PacketTypeId packet_type_id() {
    return PACKET_TYPE_GSTREAMER_BUFFER;
  }

  constexpr static const char* packet_type_name() { return "GstreamerBuffer"; }

  static Status packet_type_descriptor(const GstreamerBuffer& gstreamer_buffer,
                                       google::protobuf::Any* any) {
    if (any == nullptr) {
      return InvalidArgumentError("Given a nullptr to a google::protobuf::Any");
    }
    GstreamerBufferPacketTypeDescriptor gstreamer_buffer_packet_type_desc;
    gstreamer_buffer_packet_type_desc.set_caps_string(
        gstreamer_buffer.get_caps());
    any->PackFrom(gstreamer_buffer_packet_type_desc);
    return OkStatus();
  }
};

// Pack the Packet's payload with copy semantics.
Status PackPayload(const GstreamerBuffer&, Packet*);

// Pack the Packet's payload with move semantics.
Status PackPayload(GstreamerBuffer&&, Packet*);

// Unpack the Packet's payload with copy semantics.
Status UnpackPayload(const Packet& p, GstreamerBuffer*);

// Unpack the Packet's payload with move semantics.
Status UnpackPayload(Packet&& p, GstreamerBuffer*);

}  // namespace aistreams

#endif  // AISTREAMS_BASE_TYPES_PACKET_TYPES_GSTREAMER_BUFFER_PACKET_TYPE_H_
