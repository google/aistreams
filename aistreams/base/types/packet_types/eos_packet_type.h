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

#ifndef AISTREAMS_BASE_TYPES_PACKET_TYPES_EOS_PACKET_TYPE_H_
#define AISTREAMS_BASE_TYPES_PACKET_TYPES_EOS_PACKET_TYPE_H_

#include "aistreams/base/types/eos.h"
#include "aistreams/base/types/packet_types/packet_type_traits.h"
#include "aistreams/port/canonical_errors.h"
#include "aistreams/port/status.h"
#include "aistreams/proto/packet.pb.h"
#include "aistreams/proto/types/control_signal.pb.h"
#include "aistreams/proto/types/control_signal_packet_type_descriptor.pb.h"

namespace aistreams {

// Specialization to map Eos to Packets of type PACKET_TYPE_EOS.
template <>
struct PacketTypeTraits<Eos> {
  using value_type = Eos;
  constexpr static PacketTypeId packet_type_id() {
    return PACKET_TYPE_CONTROL_SIGNAL;
  }

  constexpr static const char* packet_type_name() { return "EOS"; }

  static Status packet_type_descriptor(const Eos& eos,
                                       google::protobuf::Any* any) {
    if (any == nullptr) {
      return InvalidArgumentError("Given a nullptr to a google::protobuf::Any");
    }
    ControlSignalPacketTypeDescriptor control_signal_packet_type_desc;
    control_signal_packet_type_desc.set_type_id(CONTROL_SIGNAL_EOS);
    any->PackFrom(control_signal_packet_type_desc);
    return OkStatus();
  }
};

// Pack the Packet's payload with copy semantics.
Status PackPayload(const Eos&, Packet*);

// Unpack the Packet's payload with copy semantics.
Status UnpackPayload(const Packet& p, Eos*);

}  // namespace aistreams

#endif  // AISTREAMS_BASE_TYPES_PACKET_TYPES_EOS_PACKET_TYPE_H_
