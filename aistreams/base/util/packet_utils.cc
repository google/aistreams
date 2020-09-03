// Copyright 2020 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "aistreams/base/util/packet_utils.h"

#include <time.h>

#include "absl/strings/str_format.h"
#include "aistreams/base/packet.h"
#include "aistreams/port/canonical_errors.h"
#include "aistreams/port/logging.h"
#include "aistreams/proto/types/control_signal_packet_type_descriptor.pb.h"

namespace aistreams {

PacketTypeId GetPacketTypeId(const Packet& p) {
  return p.header().type().type_id();
}

bool IsControlSignal(const Packet& p) {
  return GetPacketTypeId(p) == PACKET_TYPE_CONTROL_SIGNAL;
}

StatusOr<ControlSignalTypeId> GetControlSignalTypeId(const Packet& p) {
  if (!IsControlSignal(p)) {
    return InvalidArgumentError(
        absl::StrFormat("Given the non-control signal packet type %s",
                        PacketTypeId_Name(GetPacketTypeId(p))));
  }
  ControlSignalPacketTypeDescriptor control_signal_packet_type_desc;
  if (!p.header().type().type_descriptor().UnpackTo(
          &control_signal_packet_type_desc)) {
    return InvalidArgumentError(
        "Failed to Unpack the type decriptor as a "
        "ControlSignalTypeDescriptor");
  }
  return control_signal_packet_type_desc.type_id();
}

bool IsEos(const Packet& p) { return IsEos(p, nullptr); }

bool IsEos(const Packet& p, std::string* reason) {
  // Check if it is a EOS control signal.
  auto control_signal_type_statusor = GetControlSignalTypeId(p);
  if (!control_signal_type_statusor.ok() ||
      control_signal_type_statusor.ValueOrDie() != CONTROL_SIGNAL_EOS) {
    return false;
  }

  if (reason != nullptr) {
    PacketAs<Eos> packet_as(p);
    if (!packet_as.ok()) {
      LOG(ERROR) << packet_as.status();
      LOG(ERROR) << "PacketAs<Eos> failed to unpack an EOS packet";
    } else {
      Eos eos = std::move(packet_as).ValueOrDie();
      *reason = eos.reason();
    }
  }
  return true;
}

}  // namespace aistreams
