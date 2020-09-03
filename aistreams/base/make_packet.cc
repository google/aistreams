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

#include "aistreams/base/make_packet.h"

#include <utility>

#include "aistreams/base/types/eos.h"
#include "aistreams/base/types/packet_types/packet_types.h"
#include "aistreams/port/status.h"
#include "aistreams/port/status_macros.h"
#include "aistreams/port/statusor.h"

namespace aistreams {

namespace internal {

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

}  // namespace internal

StatusOr<Packet> MakeEosPacket(absl::string_view reason) {
  Eos eos;
  eos.set_reason(std::string(reason));
  return MakePacket(std::move(eos));
}

}  // namespace aistreams
