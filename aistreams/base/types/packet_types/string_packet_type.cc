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

#include "aistreams/base/types/packet_types/string_packet_type.h"

#include "absl/strings/str_format.h"
#include "aistreams/port/canonical_errors.h"
#include "aistreams/port/status.h"
#include "aistreams/port/statusor.h"
#include "aistreams/proto/packet.pb.h"

namespace aistreams {

Status PackPayload(const std::string& s, Packet* p) {
  if (p == nullptr) {
    return InvalidArgumentError("Given a nullptr to a Packet");
  }
  p->mutable_payload()->assign(s.data(), s.data() + s.size());
  return OkStatus();
}

Status PackPayload(std::string&& s, Packet* p) {
  if (p == nullptr) {
    return InvalidArgumentError("Given a nullptr to a Packet");
  }
  *p->mutable_payload() = std::move(s);
  return OkStatus();
}

Status UnpackPayload(const Packet& p, std::string* to) {
  if (to == nullptr) {
    return InvalidArgumentError("Given a nullptr to a std::string");
  }
  to->assign(p.payload().begin(), p.payload().end());
  return OkStatus();
}

Status UnpackPayload(Packet&& p, std::string* to) {
  if (to == nullptr) {
    return InvalidArgumentError("Given a nullptr to a std::string");
  }
  *to = std::move(*p.mutable_payload());
  return OkStatus();
}

}  // namespace aistreams
