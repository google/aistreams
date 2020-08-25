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

#include "aistreams/base/types/packet_types/eos_packet_type.h"

#include "absl/strings/str_format.h"
#include "aistreams/base/types/eos.h"
#include "aistreams/port/canonical_errors.h"
#include "aistreams/port/status.h"
#include "aistreams/port/statusor.h"
#include "aistreams/proto/packet.pb.h"
#include "aistreams/proto/types/control_signal.pb.h"
#include "aistreams/proto/types/control_signal_packet_type_descriptor.pb.h"

namespace aistreams {

Status ValidateTypeDescriptor(const Packet& p) {
  ControlSignalPacketTypeDescriptor control_signal_packet_type_desc;
  if (!p.header().type().type_descriptor().UnpackTo(
          &control_signal_packet_type_desc)) {
    return InvalidArgumentError(
        "Failed to Unpack the type decriptor as a "
        "ControlSignalTypeDescriptor");
  }
  if (control_signal_packet_type_desc.type_id() != CONTROL_SIGNAL_EOS) {
    return InvalidArgumentError(absl::StrFormat(
        "Given the ControlSignalTypeId %s but expected %s",
        ControlSignalTypeId_Name(control_signal_packet_type_desc.type_id()),
        ControlSignalTypeId_Name(CONTROL_SIGNAL_EOS)));
  }
  return OkStatus();
}

Status PackPayload(const Eos& eos, Packet* p) {
  if (p == nullptr) {
    return InvalidArgumentError("Given a nullptr to a Packet");
  }
  if (!eos.value().SerializeToString(p->mutable_payload())) {
    return InternalError(
        "Failed to serialize the EosValue into the packet payload");
  }
  return OkStatus();
}

Status UnpackPayload(const Packet& p, Eos* to) {
  if (to == nullptr) {
    return InvalidArgumentError("Given a nullptr to an Eos");
  }

  auto status = ValidateTypeDescriptor(p);
  if (!status.ok()) {
    LOG(ERROR) << status;
    return InvalidArgumentError(
        "Failed to validate the packet type descriptor");
  }

  EosValue eos_value;
  if (!eos_value.ParseFromString(p.payload())) {
    return InternalError("Failed to parse the Eos' value");
  }
  to->set_value(eos_value);
  return OkStatus();
}

}  // namespace aistreams
