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

#include "aistreams/c/ais_packet.h"

#include <memory>
#include <string>
#include <utility>

#include "aistreams/base/packet.h"
#include "aistreams/base/types/gstreamer_buffer.h"
#include "aistreams/c/ais_gstreamer_buffer.h"
#include "aistreams/c/ais_gstreamer_buffer_internal.h"
#include "aistreams/c/ais_packet_internal.h"
#include "aistreams/c/ais_status.h"
#include "aistreams/c/ais_status_internal.h"
#include "aistreams/port/status.h"

using aistreams::GstreamerBuffer;
using aistreams::MakePacket;
using aistreams::OkStatus;
using aistreams::Packet;

void AIS_DeletePacket(AIS_Packet* p) { delete p; }

AIS_Packet* AIS_NewPacket(AIS_Status* ais_status) {
  auto ais_packet = std::make_unique<AIS_Packet>();
  ais_status->status = OkStatus();
  return ais_packet.release();
}

AIS_Packet* AIS_NewStringPacket(const char* cstr, AIS_Status* ais_status) {
  std::string s(cstr);
  auto packet_status_or = MakePacket(std::move(s));
  if (!packet_status_or.ok()) {
    ais_status->status = packet_status_or.status();
    return nullptr;
  }

  auto ais_packet = std::make_unique<AIS_Packet>();
  ais_packet->packet = std::move(packet_status_or).ValueOrDie();
  ais_status->status = OkStatus();
  return ais_packet.release();
}

AIS_Packet* AIS_NewBytesPacket(const char* src, size_t count,
                               AIS_Status* ais_status) {
  std::string s(src, count);
  auto packet_status_or = MakePacket(std::move(s));
  if (!packet_status_or.ok()) {
    ais_status->status = packet_status_or.status();
    return nullptr;
  }

  auto ais_packet = std::make_unique<AIS_Packet>();
  ais_packet->packet = std::move(packet_status_or).ValueOrDie();
  ais_status->status = OkStatus();
  return ais_packet.release();
}

AIS_Packet* AIS_NewGstreamerBufferPacket(
    AIS_GstreamerBuffer* ais_gstreamer_buffer, AIS_Status* ais_status) {
  auto packet_status_or =
      MakePacket(std::move(ais_gstreamer_buffer->gstreamer_buffer));
  if (!packet_status_or.ok()) {
    ais_status->status = packet_status_or.status();
    return nullptr;
  }
  auto ais_packet = std::make_unique<AIS_Packet>();
  ais_packet->packet = std::move(packet_status_or).ValueOrDie();
  ais_status->status = OkStatus();
  return ais_packet.release();
}
