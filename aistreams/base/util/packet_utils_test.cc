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

#include "aistreams/base/util/packet_utils.h"

#include <memory>
#include <string>

#include "aistreams/base/packet.h"
#include "aistreams/base/types/eos.h"
#include "aistreams/base/types/gstreamer_buffer.h"
#include "aistreams/port/gtest.h"
#include "aistreams/port/logging.h"
#include "aistreams/port/status.h"
#include "aistreams/port/statusor.h"
#include "aistreams/proto/packet.pb.h"
#include "google/protobuf/struct.pb.h"

namespace aistreams {

namespace {
constexpr char kTestString[] = "hello!";
}

TEST(PacketUtilsTest, IsEosTest) {
  {
    std::string reason = "some reason";
    auto packet_status_or = MakeEosPacket(reason);
    EXPECT_TRUE(packet_status_or.ok());
    auto packet = std::move(packet_status_or).ValueOrDie();
    EXPECT_TRUE(IsControlSignal(packet));
    EXPECT_TRUE(IsEos(packet));
    std::string reason_cpy;
    EXPECT_TRUE(IsEos(packet, &reason_cpy));
    EXPECT_EQ(reason, reason_cpy);
  }
  {
    std::string caps("video/x-raw");
    std::string bytes(10, 2);
    GstreamerBuffer gstreamer_buffer;
    gstreamer_buffer.set_caps_string(caps);
    gstreamer_buffer.assign(bytes);
    auto packet_status_or = MakePacket(gstreamer_buffer);
    EXPECT_TRUE(packet_status_or.ok());
    auto packet = std::move(packet_status_or).ValueOrDie();
    EXPECT_FALSE(IsControlSignal(packet));
    EXPECT_FALSE(IsEos(packet));
  }
}

TEST(PacketUtilsTest, AddendumTest) {
  {
    Packet p;

    // Insert string addenda.
    auto status = InsertStringAddendum("string-addendum", kTestString, &p);
    EXPECT_TRUE(status.ok());
    EXPECT_EQ(p.header().addenda().size(), 1);
    std::string out_string;
    status = GetStringAddendum(p, "string-addendum", &out_string);
    EXPECT_TRUE(status.ok());
    EXPECT_EQ(out_string, kTestString);
    status =
        InsertStringAddendum("string-addendum", "this should not work", &p);
    EXPECT_FALSE(status.ok());
    EXPECT_EQ(p.header().addenda().size(), 1);
    status = GetStringAddendum(p, "string-addendum", &out_string);
    EXPECT_TRUE(status.ok());
    EXPECT_EQ(out_string, kTestString);

    // Insert protobuf addenda.
    auto packet_status_or = MakeEosPacket(kTestString);
    ASSERT_TRUE(packet_status_or.ok());
    auto proto_addendum = std::move(packet_status_or).ValueOrDie();

    status = InsertProtoAddendum("proto-addendum", proto_addendum, &p);
    EXPECT_TRUE(status.ok());
    EXPECT_EQ(p.header().addenda().size(), 2);

    Packet out_proto_addendum;
    status = GetProtoAddendum(p, "proto-addendum", &out_proto_addendum);
    EXPECT_TRUE(status.ok());
    EXPECT_EQ(proto_addendum.DebugString(), out_proto_addendum.DebugString());

    // Attempt to read with the wrong protobuf type.
    google::protobuf::Value out_val;
    status = GetProtoAddendum(p, "proto-addendum", &out_val);
    EXPECT_FALSE(status.ok());
    LOG(INFO) << status;

    // Delete addenda.
    status = DeleteAddendum("bogus", &p);
    EXPECT_TRUE(status.ok());
    EXPECT_EQ(p.header().addenda().size(), 2);
    status = DeleteAddendum("string-addendum", &p);
    EXPECT_TRUE(status.ok());
    EXPECT_EQ(p.header().addenda().size(), 1);
    status = DeleteAddendum("proto-addendum", &p);
    EXPECT_TRUE(status.ok());
    EXPECT_EQ(p.header().addenda().size(), 0);
  }
}

TEST(PacketUtilsTest, FlagsTest) {
  {
    Packet p;
    ClearPacketFlags(&p);
    EXPECT_FALSE(IsFrameHead(p));
    EXPECT_FALSE(IsKeyFrame(p));

    ClearPacketFlags(&p);
    SetPacketFlags(PacketFlags::kIsFrameHead | PacketFlags::kIsKeyFrame, &p);
    EXPECT_TRUE(IsFrameHead(p));
    EXPECT_TRUE(IsKeyFrame(p));

    ClearPacketFlags(&p);
    SetPacketFlags(PacketFlags::kIsFrameHead, &p);
    EXPECT_TRUE(IsFrameHead(p));
    EXPECT_FALSE(IsKeyFrame(p));

    ClearPacketFlags(&p);
    SetPacketFlags(PacketFlags::kIsKeyFrame, &p);
    EXPECT_FALSE(IsFrameHead(p));
    EXPECT_TRUE(IsKeyFrame(p));
  }
}

}  // namespace aistreams
