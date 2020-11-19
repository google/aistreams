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

#ifndef AISTREAMS_BASE_MAKE_PACKET_H_
#define AISTREAMS_BASE_MAKE_PACKET_H_

#include <utility>

#include "absl/strings/string_view.h"
#include "aistreams/base/packet_flags.h"
#include "aistreams/base/types/packet_types/packet_types.h"
#include "aistreams/port/status.h"
#include "aistreams/port/status_macros.h"
#include "aistreams/port/statusor.h"

namespace aistreams {

// Create a Packet from an object of type T.
//
// The created packet will have the following attributes set:
// + The timestamp will be set to the current local time.
// + The type information and payload will be set so that the Packet can
//   later be recovered as an object of type T with the given value.
template <typename T>
StatusOr<Packet> MakePacket(T&& t);

// Same as MakePacket(T&&) but with all header fields initialized to those
// provided. The fields related to recovering the type and value of the original
// object will be altered. Like MakePacket(T&&), the timestamp is also set to
// the current time.
template <typename T>
StatusOr<Packet> MakePacket(T&& t, PacketHeader header);

// Same as MakePacket(T&&, PacketHeader), but with an option to also keep the
// timestamp unaltered.
template <typename T>
StatusOr<Packet> MakePacket(T&& t, PacketHeader header, bool set_current_time);

// Create a EOS Packet with the given reason.
StatusOr<Packet> MakeEosPacket(absl::string_view reason);

// ----------------------------------------------------------------------------
// Implementation below.

namespace internal {
Status SetToCurrentTime(Packet*);
}  // namespace internal

template <typename T>
StatusOr<Packet> MakePacket(T&& t, PacketHeader header, bool set_current_time) {
  Packet p;
  *p.mutable_header() = std::move(header);
  AIS_RETURN_IF_ERROR(Pack(std::forward<T>(t), &p));
  if (set_current_time) {
    AIS_RETURN_IF_ERROR(internal::SetToCurrentTime(&p));
  }
  AIS_RETURN_IF_ERROR(RestoreDefaultPacketFlags(&p));
  return p;
}

template <typename T>
StatusOr<Packet> MakePacket(T&& t, PacketHeader header) {
  return MakePacket(std::forward<T>(t), std::move(header),
                    /* set_current_time = */ true);
};

template <typename T>
StatusOr<Packet> MakePacket(T&& t) {
  return MakePacket(std::forward<T>(t), PacketHeader(),
                    /* set_current_time = */ true);
};

}  // namespace aistreams

#endif  // AISTREAMS_BASE_MAKE_PACKET_H_
