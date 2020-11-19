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

#include "aistreams/base/packet_flags.h"

#include "aistreams/port/gtest.h"
#include "aistreams/port/logging.h"
#include "aistreams/port/status.h"
#include "aistreams/proto/packet.pb.h"

namespace aistreams {

TEST(PacketFlagsTest, BasicTests) {
  {
    // Clear flags.
    Packet p;
    ClearPacketFlags(&p);
    EXPECT_EQ(p.header().flags(), static_cast<int>(PacketFlags::kEmpty));
    EXPECT_FALSE(IsPacketFlagsSet(PacketFlags::kIsFrameHead, p));
    EXPECT_FALSE(IsPacketFlagsSet(PacketFlags::kIsKeyFrame, p));
    EXPECT_FALSE(IsPacketFlagsSet(
        PacketFlags::kIsFrameHead | PacketFlags::kIsKeyFrame, p));

    // Set flags.
    SetPacketFlags(PacketFlags::kIsFrameHead | PacketFlags::kIsKeyFrame, &p);
    EXPECT_TRUE(IsPacketFlagsSet(PacketFlags::kIsFrameHead, p));
    EXPECT_TRUE(IsPacketFlagsSet(PacketFlags::kIsKeyFrame, p));
    EXPECT_TRUE(IsPacketFlagsSet(
        PacketFlags::kIsFrameHead | PacketFlags::kIsKeyFrame, p));
    ClearPacketFlags(&p);

    SetPacketFlags(PacketFlags::kIsFrameHead, &p);
    EXPECT_TRUE(IsPacketFlagsSet(PacketFlags::kIsFrameHead, p));
    EXPECT_FALSE(IsPacketFlagsSet(PacketFlags::kIsKeyFrame, p));
    EXPECT_FALSE(IsPacketFlagsSet(
        PacketFlags::kIsFrameHead | PacketFlags::kIsKeyFrame, p));
    ClearPacketFlags(&p);

    SetPacketFlags(PacketFlags::kIsKeyFrame, &p);
    EXPECT_FALSE(IsPacketFlagsSet(PacketFlags::kIsFrameHead, p));
    EXPECT_TRUE(IsPacketFlagsSet(PacketFlags::kIsKeyFrame, p));
    EXPECT_FALSE(IsPacketFlagsSet(
        PacketFlags::kIsFrameHead | PacketFlags::kIsKeyFrame, p));
    ClearPacketFlags(&p);

    // Unset flags
    SetPacketFlags(PacketFlags::kIsFrameHead | PacketFlags::kIsKeyFrame, &p);
    UnsetPacketFlags(PacketFlags::kIsFrameHead, &p);
    EXPECT_FALSE(IsPacketFlagsSet(PacketFlags::kIsFrameHead, p));
    EXPECT_TRUE(IsPacketFlagsSet(PacketFlags::kIsKeyFrame, p));
    EXPECT_FALSE(IsPacketFlagsSet(
        PacketFlags::kIsFrameHead | PacketFlags::kIsKeyFrame, p));
    ClearPacketFlags(&p);

    SetPacketFlags(PacketFlags::kIsFrameHead | PacketFlags::kIsKeyFrame, &p);
    UnsetPacketFlags(PacketFlags::kIsKeyFrame, &p);
    EXPECT_TRUE(IsPacketFlagsSet(PacketFlags::kIsFrameHead, p));
    EXPECT_FALSE(IsPacketFlagsSet(PacketFlags::kIsKeyFrame, p));
    EXPECT_FALSE(IsPacketFlagsSet(
        PacketFlags::kIsFrameHead | PacketFlags::kIsKeyFrame, p));
    ClearPacketFlags(&p);

    SetPacketFlags(PacketFlags::kIsFrameHead | PacketFlags::kIsKeyFrame, &p);
    UnsetPacketFlags(PacketFlags::kIsFrameHead | PacketFlags::kIsKeyFrame, &p);
    EXPECT_FALSE(IsPacketFlagsSet(PacketFlags::kIsFrameHead, p));
    EXPECT_FALSE(IsPacketFlagsSet(PacketFlags::kIsKeyFrame, p));
    EXPECT_FALSE(IsPacketFlagsSet(
        PacketFlags::kIsFrameHead | PacketFlags::kIsKeyFrame, p));
    ClearPacketFlags(&p);
  }
}

}  // namespace aistreams
