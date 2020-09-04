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

#include "aistreams/c/ais_packet_as.h"

#include <memory>
#include <string>
#include <utility>

#include "aistreams/base/packet.h"
#include "aistreams/base/types/gstreamer_buffer.h"
#include "aistreams/c/ais_gstreamer_buffer.h"
#include "aistreams/c/ais_gstreamer_buffer_internal.h"
#include "aistreams/c/ais_packet.h"
#include "aistreams/c/ais_packet_as_internal.h"
#include "aistreams/c/ais_packet_internal.h"
#include "aistreams/c/ais_status.h"
#include "aistreams/c/ais_status_internal.h"
#include "aistreams/port/status.h"

using aistreams::GstreamerBuffer;
using aistreams::MakePacket;
using aistreams::OkStatus;
using aistreams::Packet;
using aistreams::PacketAs;

const void* AIS_PacketAsValue(AIS_PacketAs* ais_packet_as) {
  return ais_packet_as->ais_value_type;
}

AIS_PacketAs* AIS_NewGstreamerBufferPacketAs(AIS_Packet* ais_packet,
                                             AIS_Status* ais_status) {
  PacketAs<GstreamerBuffer> packet_as(std::move(ais_packet->packet));
  if (!packet_as.ok()) {
    ais_status->status = packet_as.status();
    return NULL;
  }
  auto ais_packet_as = std::make_unique<AIS_PacketAs>();
  ais_packet_as->packet_header = packet_as.header();
  AIS_GstreamerBuffer* ais_gstreamer_buffer = AIS_NewGstreamerBuffer();
  ais_gstreamer_buffer->gstreamer_buffer = std::move(packet_as).ValueOrDie();
  ais_packet_as->ais_value_type = static_cast<void*>(ais_gstreamer_buffer);
  ais_status->status = OkStatus();
  return ais_packet_as.release();
}

void AIS_DeleteGstreamerBufferPacketAs(AIS_PacketAs* ais_packet_as) {
  if (ais_packet_as != nullptr) {
    AIS_DeleteGstreamerBuffer(
        (AIS_GstreamerBuffer*)ais_packet_as->ais_value_type);
  }
  delete ais_packet_as;
}
