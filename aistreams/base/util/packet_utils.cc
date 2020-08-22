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
#include "aistreams/port/canonical_errors.h"
#include "aistreams/port/logging.h"
#include "aistreams/proto/types/control_signal_packet_type_descriptor.pb.h"

namespace aistreams {

Status SetToCurrentTime(Packet* p) {
  if (p == nullptr) {
    return InvalidArgumentError("Given a nullptr to a Packet");
  }
  timespec ts;
  if (clock_gettime(CLOCK_REALTIME, &ts) != 0) {
    return UnknownError("clock_gettime failed");
  }
  p->mutable_header()->mutable_timestamp()->set_seconds(ts.tv_sec);
  p->mutable_header()->mutable_timestamp()->set_nanos(ts.tv_nsec);
  return OkStatus();
}

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

bool IsEos(const Packet& p) {
  auto control_signal_type_statusor = GetControlSignalTypeId(p);
  if (!control_signal_type_statusor.ok()) {
    return false;
  }
  return control_signal_type_statusor.ValueOrDie() == CONTROL_SIGNAL_EOS;
}

}  // namespace aistreams
