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

#ifndef AISTREAMS_BASE_TYPES_PACKET_TYPES_PACKET_TYPES_H_
#define AISTREAMS_BASE_TYPES_PACKET_TYPES_PACKET_TYPES_H_

#include <type_traits>

#include "absl/strings/str_format.h"
#include "aistreams/base/types/packet_types/eos_packet_type.h"
#include "aistreams/base/types/packet_types/gstreamer_buffer_packet_type.h"
#include "aistreams/base/types/packet_types/jpeg_frame_packet_type.h"
#include "aistreams/base/types/packet_types/packet_type_traits.h"
#include "aistreams/base/types/packet_types/protobuf_packet_type.h"
#include "aistreams/base/types/packet_types/raw_image_packet_type.h"
#include "aistreams/base/types/packet_types/string_packet_type.h"
#include "aistreams/port/canonical_errors.h"
#include "aistreams/port/logging.h"
#include "aistreams/port/status.h"
#include "aistreams/port/status_macros.h"
#include "aistreams/proto/packet.pb.h"

namespace aistreams {

// Packs (marshalls) an object of a given type into a Packet.
//
// This method will only mutate the fields necessary to interpret the packet
// payload as the given type; i.e. the type_id and the type_descriptor.
template <typename T>
Status Pack(T&& t, Packet* p);

// Unpacks (un-marshalls) a Packet into an object of a given type.
//
// This method will interpret and copy the information given in the Packet into
// the object of the given type.
template <typename T>
Status Unpack(const Packet& p, T* t);
template <typename T>
Status Unpack(Packet&& p, T* t);

// --------------------------------------------------------------------
// Implementation below.

template <typename T>
Status Pack(T&& t, Packet* p) {
  if (p == nullptr) {
    return InvalidArgumentError("Given a nullptr to a Packet");
  }
  PacketType* packet_type = p->mutable_header()->mutable_type();
  packet_type->set_type_id(
      PacketTypeTraits<
          typename std::remove_reference<T>::type>::packet_type_id());
  auto type_descriptor_any_ptr = packet_type->mutable_type_descriptor();
  auto status = PacketTypeTraits<typename std::remove_reference<T>::type>::
      packet_type_descriptor(t, type_descriptor_any_ptr);
  if (!status.ok()) {
    LOG(ERROR) << status;
    return InternalError("Failed to pack the packet type descriptor");
  }
  status = PackPayload(std::forward<T>(t), p);
  if (!status.ok()) {
    LOG(ERROR) << status;
    return InternalError("Failed to pack the packet payload");
  }
  return OkStatus();
}

namespace {
template <typename T>
Status ValidateUnpackArgs(const Packet& p, T* t) {
  if (t == nullptr) {
    return InvalidArgumentError("Given a nullptr to the desitination object");
  }
  if (p.header().type().type_id() != PacketTypeTraits<T>::packet_type_id()) {
    return InvalidArgumentError(absl::StrFormat(
        "Given a Packet of type %s while the destination object requires "
        "a type of %s",
        PacketTypeId_Name(p.header().type().type_id()),
        PacketTypeId_Name(PacketTypeTraits<T>::packet_type_id())));
  }
  return OkStatus();
}
}  // namespace

template <typename T>
Status Unpack(const Packet& p, T* t) {
  AIS_RETURN_IF_ERROR(ValidateUnpackArgs(p, t));
  auto status = UnpackPayload(p, t);
  if (!status.ok()) {
    LOG(ERROR) << status;
    return UnknownError("Failed to unpack the packet payload");
  }
  return OkStatus();
}

template <typename T>
Status Unpack(Packet&& p, T* t) {
  AIS_RETURN_IF_ERROR(ValidateUnpackArgs(p, t));
  auto status = UnpackPayload(std::move(p), t);
  if (!status.ok()) {
    LOG(ERROR) << status;
    return UnknownError("Failed to unpack the packet payload");
  }
  return OkStatus();
}

}  // namespace aistreams

#endif  // AISTREAMS_BASE_TYPES_PACKET_TYPES_PACKET_TYPES_H_
