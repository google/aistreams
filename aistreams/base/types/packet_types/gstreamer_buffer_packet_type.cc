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

#include "aistreams/base/types/packet_types/gstreamer_buffer_packet_type.h"

#include "absl/strings/str_format.h"
#include "aistreams/base/types/gstreamer_buffer.h"
#include "aistreams/port/canonical_errors.h"
#include "aistreams/port/status.h"
#include "aistreams/port/statusor.h"
#include "aistreams/proto/packet.pb.h"

namespace aistreams {

namespace {

// Validate the Packet against its RawImagePacketTypeDescriptor. Return the
// corresponding RawImageDescriptor if all went well.
StatusOr<GstreamerBufferPacketTypeDescriptor> GetTypeDescriptor(
    const Packet& p) {
  GstreamerBufferPacketTypeDescriptor gstreamer_buffer_packet_type_desc;
  if (!p.header().type().type_descriptor().UnpackTo(
          &gstreamer_buffer_packet_type_desc)) {
    return InvalidArgumentError(
        "Failed to Unpack the type decriptor as a "
        "GstreamerBufferPacketTypeDescriptor");
  }
  return gstreamer_buffer_packet_type_desc;
}

}  // namespace

Status PackPayload(const GstreamerBuffer& gstreamer_buffer, Packet* p) {
  if (p == nullptr) {
    return InvalidArgumentError("Given a nullptr to a Packet");
  }
  p->mutable_payload()->assign(gstreamer_buffer.data(),
                               gstreamer_buffer.size());
  return OkStatus();
}

Status PackPayload(GstreamerBuffer&& gstreamer_buffer, Packet* p) {
  if (p == nullptr) {
    return InvalidArgumentError("Given a nullptr to a Packet");
  }
  *p->mutable_payload() = std::move(gstreamer_buffer).ReleaseBuffer();
  return OkStatus();
}

Status UnpackPayload(const Packet& p, GstreamerBuffer* to) {
  if (to == nullptr) {
    return InvalidArgumentError("Given a nullptr to a GstreamerBuffer");
  }
  auto gstreamer_packet_type_desc_statusor = GetTypeDescriptor(p);
  if (!gstreamer_packet_type_desc_statusor.ok()) {
    return gstreamer_packet_type_desc_statusor.status();
  }
  auto gstreamer_packet_type_desc =
      std::move(gstreamer_packet_type_desc_statusor).ValueOrDie();
  to->set_caps_string(gstreamer_packet_type_desc.caps_string());
  to->assign(p.payload());
  return OkStatus();
}

Status UnpackPayload(Packet&& p, GstreamerBuffer* to) {
  if (to == nullptr) {
    return InvalidArgumentError("Given a nullptr to a GstreamerBuffer");
  }
  auto gstreamer_packet_type_desc_statusor = GetTypeDescriptor(p);
  if (!gstreamer_packet_type_desc_statusor.ok()) {
    return gstreamer_packet_type_desc_statusor.status();
  }
  auto gstreamer_packet_type_desc =
      std::move(gstreamer_packet_type_desc_statusor).ValueOrDie();
  to->set_caps_string(gstreamer_packet_type_desc.caps_string());
  to->assign(std::move(*p.mutable_payload()));
  return OkStatus();
}

}  // namespace aistreams
