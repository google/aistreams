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

#ifndef AISTREAMS_BASE_TYPES_PACKET_TYPES_PROTOBUF_PACKET_TYPE_H_
#define AISTREAMS_BASE_TYPES_PACKET_TYPES_PROTOBUF_PACKET_TYPE_H_

#include <string>
#include <type_traits>

#include "aistreams/base/types/packet_types/packet_type_traits.h"
#include "aistreams/port/status.h"
#include "aistreams/proto/packet.pb.h"
#include "aistreams/proto/types/packet_type.pb.h"
#include "aistreams/proto/types/protobuf_packet_type_descriptor.pb.h"
#include "google/protobuf/any.pb.h"

namespace aistreams {

// We are using a similar pattern documented in
// https://en.cppreference.com/w/cpp/types/void_t
// to detect the presence of member functions we think are unique to protobuf
// messages. The is_protobuf_helper and conditional_t is used to simulate the
// behavior of void_t, which is only available in C++17 (we are currently on
// C++14 for this project).
//
// TODO(dschao): Consider creating the class template
// template <typename T> class Proto;
// that we can use to explicitly state the intention to interpret T as a
// protobuf. This can be then be used in CRTP, where we can enforce the static
// interface rather than the set of packet trait conventions currently used.

template <typename T, typename _ = void>
struct is_protobuf : std::false_type {};

template <typename... Ts>
struct is_protobuf_helper {};

template <typename T>
struct is_protobuf<
    T, std::conditional_t<
           false,
           is_protobuf_helper<
               decltype(std::declval<typename std::remove_const<T>::type>()
                            .ParseFromString(std::declval<std::string>())),
               decltype(std::declval<T>().SerializeToString(nullptr)),
               decltype(std::declval<T>().GetTypeName())>,
           void> > : public std::true_type {};

template <typename T>
struct PacketTypeTraits<T,
                        typename std::enable_if<is_protobuf<T>::value>::type> {
  using value_type = T;
  constexpr static PacketTypeId packet_type_id() {
    return PACKET_TYPE_PROTOBUF;
  }

  constexpr static const char* packet_type_name() {
    return "google::protobuf::Message";
  }

  static Status packet_type_descriptor(const T& t, google::protobuf::Any* any) {
    if (any == nullptr) {
      return InvalidArgumentError("Given a nullptr to a google::protobuf::Any");
    }
    ProtobufPacketTypeDescriptor protobuf_packet_type_desc;
    protobuf_packet_type_desc.set_specific_message_type_name(t.GetTypeName());
    protobuf_packet_type_desc.set_is_text_format(false);
    any->PackFrom(protobuf_packet_type_desc);
    return OkStatus();
  }
};

template <typename T,
          typename std::enable_if<is_protobuf<T>::value>::type* = nullptr>
Status PackPayload(const T& t, Packet* p) {
  if (p == nullptr) {
    return InvalidArgumentError("Given a nullptr to a Packet");
  }
  if (!t.SerializeToString(p->mutable_payload())) {
    return InternalError(
        "Failed to serialize the protobuf into the packet payload");
  }
  return OkStatus();
}

template <typename T,
          typename std::enable_if<is_protobuf<T>::value>::type* = nullptr>
Status UnpackPayload(const Packet& p, T* t) {
  if (t == nullptr) {
    return InvalidArgumentError("Given a nullptr to the destination protobuf");
  }
  ProtobufPacketTypeDescriptor protobuf_packet_type_desc;
  if (!p.header().type().type_descriptor().UnpackTo(
          &protobuf_packet_type_desc)) {
    return InvalidArgumentError(
        "Failed to Unpack the packet type decriptor as a "
        "ProtobufPacketTypeDescriptor");
  }
  if (protobuf_packet_type_desc.specific_message_type_name() !=
      t->GetTypeName()) {
    return InvalidArgumentError(absl::StrFormat(
        "Given a protobuf packet containing the specific type %s, "
        "but we are trying to receive it with a protobuf of type %s",
        protobuf_packet_type_desc.specific_message_type_name(),
        t->GetTypeName()));
  }
  if (protobuf_packet_type_desc.is_text_format()) {
    return InvalidArgumentError(
        "Given a text format encoded protobuf. We require a binary encoded one "
        "here");
  }
  if (!t->ParseFromString(p.payload())) {
    return InternalError("Failed to parse the protobuf payload");
  }
  return OkStatus();
}

}  // namespace aistreams

#endif  // AISTREAMS_BASE_TYPES_PACKET_TYPES_PROTOBUF_PACKET_TYPE_H_
