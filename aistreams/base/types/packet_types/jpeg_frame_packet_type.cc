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

#include "aistreams/base/types/packet_types/jpeg_frame_packet_type.h"

#include "absl/strings/str_format.h"
#include "aistreams/base/types/jpeg_frame.h"
#include "aistreams/port/canonical_errors.h"
#include "aistreams/port/status.h"
#include "aistreams/port/statusor.h"
#include "aistreams/proto/packet.pb.h"

namespace aistreams {

Status PackPayload(const JpegFrame& jpeg_frame, Packet* p) {
  if (p == nullptr) {
    return InvalidArgumentError("Given a nullptr to a Packet");
  }
  p->mutable_payload()->assign(jpeg_frame.data(),
                               jpeg_frame.data() + jpeg_frame.size());
  return OkStatus();
}

Status PackPayload(JpegFrame&& jpeg_frame, Packet* p) {
  if (p == nullptr) {
    return InvalidArgumentError("Given a nullptr to a Packet");
  }
  *p->mutable_payload() = std::move(jpeg_frame).ReleaseBuffer();
  return OkStatus();
}

Status UnpackPayload(const Packet& p, JpegFrame* to) {
  if (to == nullptr) {
    return InvalidArgumentError("Given a nullptr to a JpegFrame");
  }
  std::string s(p.payload().begin(), p.payload().end());
  JpegFrame from(std::move(s));
  *to = std::move(from);
  return OkStatus();
}

Status UnpackPayload(Packet&& p, JpegFrame* to) {
  if (to == nullptr) {
    return InvalidArgumentError("Given a nullptr to a JpegFrame");
  }
  *to = JpegFrame(std::move(*p.mutable_payload()));
  return OkStatus();
}

}  // namespace aistreams
