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

#include <string.h>

#include <memory>
#include <string>
#include <utility>

#include "aistreams/base/packet.h"
#include "aistreams/base/packet_flags.h"
#include "aistreams/base/types/gstreamer_buffer.h"
#include "aistreams/base/util/packet_utils.h"
#include "aistreams/c/ais_gstreamer_buffer.h"
#include "aistreams/c/ais_gstreamer_buffer_internal.h"
#include "aistreams/c/ais_packet_internal.h"
#include "aistreams/c/ais_status.h"
#include "aistreams/c/ais_status_internal.h"
#include "aistreams/port/status.h"

using aistreams::GstreamerBuffer;
using aistreams::IsEos;
using aistreams::MakeEosPacket;
using aistreams::MakePacket;
using aistreams::OkStatus;
using aistreams::Packet;
using aistreams::PacketFlags;
using aistreams::SetPacketFlags;
using aistreams::UnsetPacketFlags;

void AIS_DeletePacket(AIS_Packet* p) { delete p; }

AIS_Packet* AIS_NewPacket(AIS_Status* ais_status) {
  auto ais_packet = std::make_unique<AIS_Packet>();
  ais_status->status = OkStatus();
  return ais_packet.release();
}

extern AIS_Packet* AIS_NewEosPacket(const char* reason,
                                    AIS_Status* ais_status) {
  auto packet_status_or = MakeEosPacket(reason);
  if (!packet_status_or.ok()) {
    ais_status->status = packet_status_or.status();
    return nullptr;
  }

  auto ais_packet = std::make_unique<AIS_Packet>();
  ais_packet->packet = std::move(packet_status_or).ValueOrDie();
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

unsigned char AIS_IsEos(const AIS_Packet* ais_packet, char** reason) {
  if (reason == nullptr) {
    return static_cast<unsigned char>(IsEos(ais_packet->packet));
  }

  std::string reason_string;
  auto is_eos =
      static_cast<unsigned char>(IsEos(ais_packet->packet, &reason_string));
  *reason = strdup(reason_string.c_str());
  return is_eos;
}

void AIS_SetIsKeyFrame(unsigned char is_key_frame, AIS_Packet* ais_packet) {
  if (is_key_frame) {
    SetPacketFlags(PacketFlags::kIsKeyFrame, &ais_packet->packet);
  } else {
    UnsetPacketFlags(PacketFlags::kIsKeyFrame, &ais_packet->packet);
  }
  return;
}

void AIS_SetIsFrameHead(unsigned char is_frame_head, AIS_Packet* ais_packet) {
  if (is_frame_head) {
    SetPacketFlags(PacketFlags::kIsFrameHead, &ais_packet->packet);
  } else {
    UnsetPacketFlags(PacketFlags::kIsFrameHead, &ais_packet->packet);
  }
  return;
}
