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

#include "aistreams/gstreamer/ais_type_utils.h"

#include <memory>
#include <string>
#include <utility>

#include "aistreams/c/ais_gstreamer_buffer_internal.h"
#include "aistreams/c/ais_packet_internal.h"
#include "aistreams/c/ais_status_internal.h"
#include "aistreams/gstreamer/type_utils.h"
#include "aistreams/port/status.h"

using aistreams::OkStatus;
using aistreams::ToGstreamerBuffer;

AIS_GstreamerBuffer* AIS_ToGstreamerBuffer(AIS_Packet* ais_packet,
                                           AIS_Status* ais_status) {
  auto gstreamer_buffer_statusor =
      ToGstreamerBuffer(std::move(ais_packet->packet));
  if (!gstreamer_buffer_statusor.ok()) {
    ais_status->status = gstreamer_buffer_statusor.status();
    return nullptr;
  }
  auto ais_gstreamer_buffer = std::make_unique<AIS_GstreamerBuffer>();
  ais_gstreamer_buffer->gstreamer_buffer =
      std::move(gstreamer_buffer_statusor).ValueOrDie();
  ais_status->status = OkStatus();
  return ais_gstreamer_buffer.release();
}
