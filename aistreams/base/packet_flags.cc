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

#include "aistreams/base/packet_flags.h"

#include "aistreams/port/canonical_errors.h"

namespace aistreams {

void SetPacketFlags(PacketFlags pflags, Packet* p) {
  int current_flags = p->header().flags();
  int flags = static_cast<int>(pflags);
  p->mutable_header()->set_flags(current_flags | flags);
}

void UnsetPacketFlags(PacketFlags pflags, Packet* p) {
  int current_flags = p->header().flags();
  int flags = static_cast<int>(pflags);
  p->mutable_header()->set_flags(current_flags & ~flags);
}

bool IsPacketFlagsSet(PacketFlags pflags, const Packet& p) {
  int current_flags = p.header().flags();
  int flags = static_cast<int>(pflags);
  return (current_flags & flags) == flags;
}

void ClearPacketFlags(Packet* p) {
  p->mutable_header()->set_flags(static_cast<int>(PacketFlags::kEmpty));
}

Status RestoreDefaultPacketFlags(Packet* p) {
  if (p == nullptr) {
    return InvalidArgumentError("Given a nullptr to a Packet");
  }
  ClearPacketFlags(p);
  PacketTypeId type_id = p->header().type().type_id();
  switch (type_id) {
    case PACKET_TYPE_JPEG:
    case PACKET_TYPE_RAW_IMAGE:
    case PACKET_TYPE_PROTOBUF:
    case PACKET_TYPE_STRING:
    case PACKET_TYPE_GSTREAMER_BUFFER:
      SetPacketFlags(PacketFlags::kIsFrameHead | PacketFlags::kIsKeyFrame, p);
      break;
    case PACKET_TYPE_CONTROL_SIGNAL:
    default:
      break;
  }
  return OkStatus();
}

}  // namespace aistreams
