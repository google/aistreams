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

#include <cstring>
#include <string>

#include "aistreams/c/ais_gstreamer_buffer.h"
#include "aistreams/c/ais_gstreamer_buffer_internal.h"
#include "aistreams/c/ais_packet_as.h"
#include "aistreams/c/ais_packet_as_internal.h"
#include "aistreams/c/ais_packet_internal.h"
#include "aistreams/c/ais_status.h"
#include "aistreams/c/ais_status_internal.h"
#include "aistreams/port/gtest.h"
#include "aistreams/port/logging.h"
#include "aistreams/port/status.h"
#include "aistreams/port/statusor.h"

namespace aistreams {

TEST(CAPI, AIS_PacketTest) {
  AIS_Status* ais_status = AIS_NewStatus();

  AIS_GstreamerBuffer* ais_gstreamer_buffer_src = AIS_NewGstreamerBuffer();
  AIS_GstreamerBufferSetCapsString("video/x-raw", ais_gstreamer_buffer_src);
  std::string src = "hello";
  AIS_GstreamerBufferAssign(src.c_str(), src.size(), ais_gstreamer_buffer_src);

  AIS_Packet* ais_packet =
      AIS_NewGstreamerBufferPacket(ais_gstreamer_buffer_src, ais_status);
  AIS_DeleteGstreamerBuffer(ais_gstreamer_buffer_src);
  EXPECT_NE(ais_packet, nullptr);

  AIS_PacketAs* ais_packet_as =
      AIS_NewGstreamerBufferPacketAs(ais_packet, ais_status);
  AIS_DeletePacket(ais_packet);
  EXPECT_NE(ais_packet_as, nullptr);

  const AIS_GstreamerBuffer* ais_gstreamer_buffer_dst =
      (const AIS_GstreamerBuffer*)AIS_PacketAsValue(ais_packet_as);
  EXPECT_EQ(strcmp(AIS_GstreamerBufferGetCapsString(ais_gstreamer_buffer_dst),
                   "video/x-raw"),
            0);
  std::string dst(AIS_GstreamerBufferSize(ais_gstreamer_buffer_dst), '.');
  AIS_GstreamerBufferCopyTo(ais_gstreamer_buffer_dst,
                            const_cast<char*>(dst.data()));
  EXPECT_EQ(dst, src);

  AIS_DeleteGstreamerBufferPacketAs(ais_packet_as);
  AIS_DeleteStatus(ais_status);
}

TEST(CAPI, AIS_PacketEosTest) {
  {
    AIS_Status* ais_status = AIS_NewStatus();
    std::string src_reason = "some reason";
    AIS_Packet* ais_packet = AIS_NewEosPacket(src_reason.c_str(), ais_status);
    EXPECT_NE(ais_packet, nullptr);
    char* dst_reason_cstr;
    EXPECT_TRUE(AIS_IsEos(ais_packet, &dst_reason_cstr));
    std::string dst_reason(dst_reason_cstr);
    free(dst_reason_cstr);
    AIS_DeletePacket(ais_packet);
    AIS_DeleteStatus(ais_status);
    EXPECT_EQ(src_reason, dst_reason);
  }
  {
    AIS_Status* ais_status = AIS_NewStatus();
    AIS_GstreamerBuffer* ais_gstreamer_buffer_src = AIS_NewGstreamerBuffer();
    AIS_GstreamerBufferSetCapsString("video/x-raw", ais_gstreamer_buffer_src);
    std::string src = "hello";
    AIS_GstreamerBufferAssign(src.c_str(), src.size(),
                              ais_gstreamer_buffer_src);
    AIS_Packet* ais_packet =
        AIS_NewGstreamerBufferPacket(ais_gstreamer_buffer_src, ais_status);
    AIS_DeleteGstreamerBuffer(ais_gstreamer_buffer_src);
    EXPECT_NE(ais_packet, nullptr);

    EXPECT_FALSE(AIS_IsEos(ais_packet, nullptr));

    AIS_DeletePacket(ais_packet);
    AIS_DeleteStatus(ais_status);
  }
}

}  // namespace aistreams
