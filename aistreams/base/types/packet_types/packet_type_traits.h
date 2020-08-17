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

#ifndef AISTREAMS_BASE_TYPES_PACKET_TYPES_PACKET_TYPE_TRAITS_H_
#define AISTREAMS_BASE_TYPES_PACKET_TYPES_PACKET_TYPE_TRAITS_H_

#include "aistreams/port/status.h"
#include "aistreams/proto/types/packet_type.pb.h"
#include "google/protobuf/any.pb.h"

namespace aistreams {

// To add a new packet type, do the following:
// 1. Specialize the class template PacketTypeTraits and supply implementations
//    for each of the static functions.
// 2. Supply at least the following two functions:
//    + Status PackPayload(const YourCppType&, Packet*);
//    + Status UnpackPayload(const Packet&, YourCppType*);
//    You may optionally supply overloads alongside the above that accept RValue
//    references in the first parameter.
//
// TODO(dschao): Add a README to more clearly explain the procedure and
// rationale of the above. Also, consider CRTP to enforce the static interface.

template <typename T>
struct dependent_false : std::false_type {};

// Traits class to map a C++ type to a packet type.
template <typename T, typename Enable = void>
struct PacketTypeTraits {
  static_assert(dependent_false<T>::value,
                "Only specializations of PacketTypeTraits may be used.");

  using value_type = T;

  // This is the PacketTypeId enum that type T corresponds to.
  constexpr static PacketTypeId packet_type_id() { return PACKET_TYPE_UNKNOWN; }

  // This is a human readable name for the packet type.
  constexpr static const char* packet_type_name() { return "Unknown"; }

  // This populates the packet type descriptor for type T.
  //
  // Notes:
  // + Return OkStatus() if your type doesn't need a type descriptor. Also, do
  //   not modify the Any argument in this case.
  // + You can ignore the first parameter in your implementation if you do not
  //   require an actual object of type T to determine the type descriptor.
  static Status packet_type_descriptor(const T&, google::protobuf::Any*) {
    return OkStatus();
  }
};

}  // namespace aistreams

#endif  // AISTREAMS_BASE_TYPES_PACKET_TYPES_PACKET_TYPE_TRAITS_H_
