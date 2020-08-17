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

#ifndef AISTREAMS_BASE_TYPES_PACKET_TYPES_RAW_IMAGE_PACKET_TYPE_H_
#define AISTREAMS_BASE_TYPES_PACKET_TYPES_RAW_IMAGE_PACKET_TYPE_H_

#include "aistreams/base/types/packet_types/packet_type_traits.h"
#include "aistreams/base/types/raw_image.h"
#include "aistreams/port/canonical_errors.h"
#include "aistreams/port/status.h"
#include "aistreams/proto/packet.pb.h"
#include "aistreams/proto/types/raw_image.pb.h"
#include "aistreams/proto/types/raw_image_packet_type_descriptor.pb.h"

namespace aistreams {

// Specialization to map RawImage to Packets of type PACKET_TYPE_RAW_IMAGE.
template <>
struct PacketTypeTraits<RawImage> {
  using value_type = RawImage;
  constexpr static PacketTypeId packet_type_id() {
    return PACKET_TYPE_RAW_IMAGE;
  }

  constexpr static const char* packet_type_name() { return "RawImage"; }

  static Status packet_type_descriptor(const RawImage& raw_image,
                                       google::protobuf::Any* any) {
    if (any == nullptr) {
      return InvalidArgumentError("Given a nullptr to a google::protobuf::Any");
    }
    RawImagePacketTypeDescriptor raw_image_packet_type_desc;
    raw_image_packet_type_desc.mutable_raw_image_descriptor()->set_format(
        raw_image.format());
    raw_image_packet_type_desc.mutable_raw_image_descriptor()->set_height(
        raw_image.height());
    raw_image_packet_type_desc.mutable_raw_image_descriptor()->set_width(
        raw_image.width());
    any->PackFrom(raw_image_packet_type_desc);
    return OkStatus();
  }
};

// Pack the Packet's payload with copy semantics.
Status PackPayload(const RawImage& raw_image, Packet* p);

// Pack the Packet's payload with move semantics.
Status PackPayload(RawImage&& raw_image, Packet* p);

// Unpack the Packet's payload with copy semantics.
Status UnpackPayload(const Packet& p, RawImage* to);

// Unpack the Packet's payload with move semantics.
Status UnpackPayload(Packet&& p, RawImage* to);

}  // namespace aistreams

#endif  // AISTREAMS_BASE_TYPES_PACKET_TYPES_RAW_IMAGE_PACKET_TYPE_H_
