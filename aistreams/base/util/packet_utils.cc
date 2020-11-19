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
#include "aistreams/base/packet_flags.h"
#include "aistreams/port/canonical_errors.h"
#include "aistreams/port/logging.h"
#include "aistreams/proto/types/control_signal_packet_type_descriptor.pb.h"
#include "google/protobuf/any.pb.h"
#include "google/protobuf/struct.pb.h"

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

namespace {

Status InsertAddendum(const std::string& key, google::protobuf::Any value,
                      Packet* packet) {
  auto* addenda = packet->mutable_header()->mutable_addenda();
  auto p = addenda->insert(
      typename std::remove_reference<decltype(*addenda)>::type::value_type(
          key, std::move(value)));
  if (!p.second) {
    return InvalidArgumentError(
        absl::StrFormat("An addendum with key \"%s\" already exists", key));
  }
  return OkStatus();
}

Status GetAddendum(const Packet& packet, const std::string& key,
                   google::protobuf::Any* any) {
  auto addenda = packet.header().addenda();
  auto it = addenda.find(key);
  if (it == addenda.end()) {
    return InvalidArgumentError(
        absl::StrFormat("Failed to find an addendum for key \"%s\"", key));
  }
  *any = it->second;
  return OkStatus();
}

}  // namespace

Status InsertProtoAddendum(const std::string& key,
                           const google::protobuf::Message& value,
                           Packet* packet) {
  google::protobuf::Any any;
  any.PackFrom(value);
  return InsertAddendum(key, std::move(any), packet);
}

Status InsertStringAddendum(const std::string& key, const std::string& value,
                            Packet* packet) {
  google::protobuf::Value v;
  v.set_string_value(value);
  return InsertProtoAddendum(key, v, packet);
}

Status GetStringAddendum(const Packet& packet, const std::string& key,
                         std::string* s) {
  google::protobuf::Value v;
  auto status = GetProtoAddendum(packet, key, &v);
  if (!status.ok()) {
    return status;
  }
  if (v.kind_case() != google::protobuf::Value::kStringValue) {
    return InvalidArgumentError(
        absl::StrFormat("Key \"%s\" does not contain a string value", key));
  }
  *s = v.string_value();
  return OkStatus();
}

Status GetProtoAddendum(const Packet& p, const std::string& key,
                        google::protobuf::Message* m) {
  google::protobuf::Any any;
  auto status = GetAddendum(p, key, &any);
  if (!status.ok()) {
    return status;
  }
  if (!any.UnpackTo(m)) {
    return UnknownError(
        absl::StrFormat("Could not unpack the addendum for key \"%s\" as a %s",
                        key, m->GetTypeName()));
  }
  return OkStatus();
}

Status DeleteAddendum(const std::string& key, Packet* packet) {
  auto* addenda = packet->mutable_header()->mutable_addenda();
  addenda->erase(key);
  return OkStatus();
}

bool IsKeyFrame(const Packet& p) {
  return IsPacketFlagsSet(PacketFlags::kIsKeyFrame, p);
}

bool IsFrameHead(const Packet& p) {
  return IsPacketFlagsSet(PacketFlags::kIsFrameHead, p);
}

}  // namespace aistreams
