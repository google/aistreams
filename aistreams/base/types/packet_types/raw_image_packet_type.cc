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

#include "aistreams/base/types/packet_types/raw_image_packet_type.h"

#include "absl/strings/str_format.h"
#include "aistreams/base/types/raw_image.h"
#include "aistreams/base/types/raw_image_helpers.h"
#include "aistreams/port/status.h"
#include "aistreams/port/statusor.h"
#include "aistreams/proto/packet.pb.h"
#include "aistreams/proto/types/raw_image_packet_type_descriptor.pb.h"

namespace aistreams {

namespace {

// Validate the Packet against its RawImagePacketTypeDescriptor. Return the
// corresponding RawImageDescriptor if all went well.
StatusOr<RawImageDescriptor> ValidateAndGetDescriptor(const Packet& p) {
  RawImagePacketTypeDescriptor raw_image_packet_type_desc;
  if (!p.header().type().type_descriptor().UnpackTo(
          &raw_image_packet_type_desc)) {
    return InvalidArgumentError(
        "Failed to Unpack the type decriptor as a "
        "RawImagePacketTypeDescriptor");
  }
  RawImageDescriptor raw_image_descriptor =
      raw_image_packet_type_desc.raw_image_descriptor();
  Status s = Validate(raw_image_descriptor);
  if (!s.ok()) {
    LOG(ERROR) << s;
    return InvalidArgumentError("Given an invalid RawImageDescriptor");
  }

  auto expected_payload_size_statusor = GetBufferSize(raw_image_descriptor);
  if (!expected_payload_size_statusor.ok()) {
    return expected_payload_size_statusor.status();
  }
  auto expected_payload_size =
      std::move(expected_payload_size_statusor).ValueOrDie();
  if (p.payload().size() != static_cast<size_t>(expected_payload_size)) {
    return InvalidArgumentError(absl::StrFormat(
        "The given Packet's payload size is inconsistent with its "
        "RawImageDescriptor (%d vs %d)",
        p.payload().size(), expected_payload_size));
  }
  return raw_image_descriptor;
}

}  // namespace

Status PackPayload(const RawImage& raw_image, Packet* p) {
  if (p == nullptr) {
    return InvalidArgumentError("Given a nullptr to a Packet");
  }
  p->mutable_payload()->assign(raw_image.data(),
                               raw_image.data() + raw_image.size());
  return OkStatus();
}

Status PackPayload(RawImage&& raw_image, Packet* p) {
  if (p == nullptr) {
    return InvalidArgumentError("Given a nullptr to a Packet");
  }
  *p->mutable_payload() = std::move(raw_image).ReleaseBuffer();
  return OkStatus();
}

Status UnpackPayload(const Packet& p, RawImage* to) {
  if (to == nullptr) {
    return InvalidArgumentError("Given a nullptr to a RawImage");
  }

  auto raw_image_desc_status_or = ValidateAndGetDescriptor(p);
  if (!raw_image_desc_status_or.ok()) {
    return raw_image_desc_status_or.status();
  }
  RawImageDescriptor raw_image_descriptor =
      raw_image_desc_status_or.ValueOrDie();

  RawImage from(raw_image_descriptor);
  std::copy(p.payload().begin(), p.payload().end(), from.data());
  *to = std::move(from);
  return OkStatus();
}

Status UnpackPayload(Packet&& p, RawImage* to) {
  if (to == nullptr) {
    return InvalidArgumentError("Given a nullptr to a RawImage");
  }

  auto raw_image_desc_status_or = ValidateAndGetDescriptor(p);
  if (!raw_image_desc_status_or.ok()) {
    return raw_image_desc_status_or.status();
  }
  RawImageDescriptor raw_image_descriptor =
      raw_image_desc_status_or.ValueOrDie();

  *to = RawImage(raw_image_descriptor, std::move(*p.mutable_payload()));
  return OkStatus();
}

}  // namespace aistreams
