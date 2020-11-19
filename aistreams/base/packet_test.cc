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

#include "aistreams/base/packet.h"

#include <memory>
#include <string>

#include "aistreams/base/types/eos.h"
#include "aistreams/base/types/gstreamer_buffer.h"
#include "aistreams/base/types/jpeg_frame.h"
#include "aistreams/base/types/raw_image.h"
#include "aistreams/port/gtest.h"
#include "aistreams/port/logging.h"
#include "aistreams/port/status.h"
#include "aistreams/port/statusor.h"
#include "aistreams/proto/packet.pb.h"
#include "aistreams/proto/types/control_signal.pb.h"
#include "aistreams/proto/types/raw_image.pb.h"
#include "aistreams/proto/types/raw_image_packet_type_descriptor.pb.h"

namespace aistreams {

TEST(PacketTest, MakePacketStringTest) {
  {
    std::string s("hello!");
    auto packet_status_or = MakePacket(s);
    EXPECT_TRUE(packet_status_or.ok());
    auto packet = std::move(packet_status_or).ValueOrDie();
    EXPECT_EQ(packet.header().type().type_id(), PACKET_TYPE_STRING);
    EXPECT_EQ(packet.payload(), s);
    EXPECT_TRUE(IsPacketFlagsSet(
        PacketFlags::kIsFrameHead | PacketFlags::kIsKeyFrame, packet));
  }
  {
    std::string s("hello!");
    std::string tmp(s);
    auto packet_status_or = MakePacket(std::move(tmp));
    EXPECT_TRUE(packet_status_or.ok());
    auto packet = std::move(packet_status_or).ValueOrDie();
    EXPECT_EQ(packet.header().type().type_id(), PACKET_TYPE_STRING);
    EXPECT_EQ(packet.payload(), s);
  }
}

TEST(PacketTest, PacketAsStringTest) {
  {
    std::string src("hey!");
    auto packet_status_or = MakePacket(src);
    EXPECT_TRUE(packet_status_or.ok());

    PacketAs<std::string> packet_as(std::move(packet_status_or).ValueOrDie());
    EXPECT_TRUE(packet_as.ok());
    PacketHeader header = packet_as.header();
    EXPECT_EQ(header.type().type_id(), PACKET_TYPE_STRING);

    std::string dst = std::move(packet_as).ValueOrDie();
    EXPECT_EQ(dst, src);
  }
}

TEST(PacketTest, MakePacketRawImageTest) {
  {
    RawImage r(2, 3, RAW_IMAGE_FORMAT_SRGB);
    r(0) = 3;
    r(1) = 1;
    r(2) = 4;
    auto packet_status_or = MakePacket(r);
    EXPECT_TRUE(packet_status_or.ok());
    auto packet = std::move(packet_status_or).ValueOrDie();
    EXPECT_EQ(packet.header().type().type_id(), PACKET_TYPE_RAW_IMAGE);
    RawImagePacketTypeDescriptor raw_image_packet_type_desc;
    EXPECT_TRUE(packet.header().type().type_descriptor().UnpackTo(
        &raw_image_packet_type_desc));
    RawImageDescriptor raw_image_descriptor =
        raw_image_packet_type_desc.raw_image_descriptor();
    EXPECT_EQ(raw_image_descriptor.height(), 2);
    EXPECT_EQ(raw_image_descriptor.width(), 3);
    EXPECT_EQ(raw_image_descriptor.format(), RAW_IMAGE_FORMAT_SRGB);
    EXPECT_EQ(packet.payload()[0], 3);
    EXPECT_EQ(packet.payload()[1], 1);
    EXPECT_EQ(packet.payload()[2], 4);
    EXPECT_EQ(packet.payload().size(), 18);
    EXPECT_TRUE(IsPacketFlagsSet(
        PacketFlags::kIsFrameHead | PacketFlags::kIsKeyFrame, packet));
  }
}

TEST(PacketTest, PacketAsRawImageTest) {
  {
    RawImage src(2, 3, RAW_IMAGE_FORMAT_SRGB);
    src(0) = 3;
    src(1) = 1;
    src(2) = 4;
    auto packet_status_or = MakePacket(src);
    EXPECT_TRUE(packet_status_or.ok());

    PacketAs<RawImage> packet_as(std::move(packet_status_or).ValueOrDie());
    EXPECT_TRUE(packet_as.ok());
    PacketHeader header = packet_as.header();
    EXPECT_EQ(header.type().type_id(), PACKET_TYPE_RAW_IMAGE);
    RawImagePacketTypeDescriptor raw_image_packet_type_desc;
    EXPECT_TRUE(
        header.type().type_descriptor().UnpackTo(&raw_image_packet_type_desc));
    RawImageDescriptor raw_image_descriptor =
        raw_image_packet_type_desc.raw_image_descriptor();
    EXPECT_EQ(raw_image_descriptor.height(), 2);
    EXPECT_EQ(raw_image_descriptor.width(), 3);
    EXPECT_EQ(raw_image_descriptor.format(), RAW_IMAGE_FORMAT_SRGB);

    RawImage dst = std::move(packet_as).ValueOrDie();
    EXPECT_EQ(dst.height(), src.height());
    EXPECT_EQ(dst.width(), src.width());
    EXPECT_EQ(dst.channels(), src.channels());
    EXPECT_EQ(dst.format(), src.format());
    EXPECT_EQ(dst.size(), src.size());
    for (size_t i = 0; i < dst.size(); ++i) {
      EXPECT_EQ(dst(i), src(i));
    }
  }
  {
    RawImage src(2, 3, RAW_IMAGE_FORMAT_SRGB);
    auto packet_status_or = MakePacket(src);
    EXPECT_TRUE(packet_status_or.ok());

    PacketAs<std::string> packet_as(std::move(packet_status_or).ValueOrDie());
    EXPECT_FALSE(packet_as.ok());
    ASSERT_DEATH(packet_as.ValueOrDie(), "");
  }
}

TEST(PacketTest, MakePacketJpegFrameTest) {
  {
    std::string bytes(10, 2);
    auto packet_status_or = MakePacket(JpegFrame(bytes));
    EXPECT_TRUE(packet_status_or.ok());
    auto packet = std::move(packet_status_or).ValueOrDie();
    EXPECT_EQ(packet.header().type().type_id(), PACKET_TYPE_JPEG);
    for (size_t i = 0; i < bytes.size(); ++i) {
      EXPECT_EQ(packet.payload()[i], bytes[i]);
    }
    EXPECT_EQ(packet.payload().size(), bytes.size());
    EXPECT_TRUE(IsPacketFlagsSet(
        PacketFlags::kIsFrameHead | PacketFlags::kIsKeyFrame, packet));
  }
}

TEST(PacketTest, PacketAsJpegFrameTest) {
  {
    std::string src(10, 2);
    auto packet_status_or = MakePacket(JpegFrame(src));
    EXPECT_TRUE(packet_status_or.ok());

    PacketAs<JpegFrame> packet_as(std::move(packet_status_or).ValueOrDie());
    EXPECT_TRUE(packet_as.ok());
    PacketHeader header = packet_as.header();
    EXPECT_EQ(header.type().type_id(), PACKET_TYPE_JPEG);

    JpegFrame dst = std::move(packet_as).ValueOrDie();
    EXPECT_EQ(dst.size(), src.size());
    for (size_t i = 0; i < dst.size(); ++i) {
      EXPECT_EQ(dst.data()[i], src[i]);
    }
  }
}

TEST(PacketTest, ProtobufPacketTest) {
  {
    // Packing a string Packet into a protobuf Packet.
    Packet src_string_packet;
    {
      std::string s("hello!");
      auto packet_status_or = MakePacket(s);
      EXPECT_TRUE(packet_status_or.ok());
      src_string_packet = std::move(packet_status_or).ValueOrDie();
    }
    auto packet_status_or = MakePacket(src_string_packet);
    EXPECT_TRUE(packet_status_or.ok());
    auto protobuf_packet = std::move(packet_status_or).ValueOrDie();
    EXPECT_EQ(protobuf_packet.header().type().type_id(), PACKET_TYPE_PROTOBUF);
    EXPECT_TRUE(IsPacketFlagsSet(
        PacketFlags::kIsFrameHead | PacketFlags::kIsKeyFrame, protobuf_packet));

    // Reading out the string Packet from the protobuf Packet.
    PacketAs<Packet> packet_as(protobuf_packet);
    EXPECT_TRUE(packet_as.ok());
    Packet dst_string_packet = std::move(packet_as).ValueOrDie();
    EXPECT_EQ(dst_string_packet.header().type().type_id(), PACKET_TYPE_STRING);
    EXPECT_EQ(dst_string_packet.payload(), src_string_packet.payload());
  }
  {
    // Create a protobuf Packet containing a Packet protobuf.
    Packet string_packet;
    {
      std::string s("hello!");
      auto packet_status_or = MakePacket(s);
      EXPECT_TRUE(packet_status_or.ok());
      string_packet = std::move(packet_status_or).ValueOrDie();
    }
    auto packet_status_or = MakePacket(string_packet);
    EXPECT_TRUE(packet_status_or.ok());
    auto protobuf_packet = std::move(packet_status_or).ValueOrDie();

    // Test that it is not possible to read it out as another protobuf message.
    PacketAs<RawImageDescriptor> packet_as(protobuf_packet);
    EXPECT_FALSE(packet_as.ok());
    ASSERT_DEATH(packet_as.ValueOrDie(), "");
  }
  {
    std::string s("hello!");
    auto packet_status_or = MakePacket(s);
    EXPECT_TRUE(packet_status_or.ok());
    const Packet string_packet = std::move(packet_status_or).ValueOrDie();

    packet_status_or = MakePacket(string_packet);
    EXPECT_TRUE(packet_status_or.ok());
    auto protobuf_packet = std::move(packet_status_or).ValueOrDie();

    // Test that it is not possible to read it out as another protobuf
    // message.
    PacketAs<RawImageDescriptor> packet_as(protobuf_packet);
    EXPECT_FALSE(packet_as.ok());
    ASSERT_DEATH(packet_as.ValueOrDie(), "");
  }
}

TEST(PacketTest, MakePacketGstreamerBufferTest) {
  {
    std::string caps("video/x-raw");
    std::string bytes(10, 2);
    GstreamerBuffer gstreamer_buffer;
    gstreamer_buffer.set_caps_string(caps);
    gstreamer_buffer.assign(bytes);
    auto packet_status_or = MakePacket(gstreamer_buffer);
    EXPECT_TRUE(packet_status_or.ok());
    auto packet = std::move(packet_status_or).ValueOrDie();
    EXPECT_EQ(packet.header().type().type_id(), PACKET_TYPE_GSTREAMER_BUFFER);
    for (size_t i = 0; i < bytes.size(); ++i) {
      EXPECT_EQ(packet.payload()[i], bytes[i]);
    }
    EXPECT_EQ(packet.payload().size(), bytes.size());
    EXPECT_TRUE(IsPacketFlagsSet(
        PacketFlags::kIsFrameHead | PacketFlags::kIsKeyFrame, packet));

    GstreamerBufferPacketTypeDescriptor gstreamer_buffer_packet_type_desc;
    EXPECT_TRUE(packet.header().type().type_descriptor().UnpackTo(
        &gstreamer_buffer_packet_type_desc));
    EXPECT_EQ(gstreamer_buffer_packet_type_desc.caps_string(), caps);
  }
  {
    std::string caps("video/x-raw");
    std::string bytes(10, 2);
    GstreamerBuffer gstreamer_buffer;
    gstreamer_buffer.set_caps_string(caps);
    gstreamer_buffer.assign(bytes);
    auto packet_status_or = MakePacket(std::move(gstreamer_buffer));
    EXPECT_TRUE(packet_status_or.ok());
    auto packet = std::move(packet_status_or).ValueOrDie();
    EXPECT_EQ(packet.header().type().type_id(), PACKET_TYPE_GSTREAMER_BUFFER);
    for (size_t i = 0; i < bytes.size(); ++i) {
      EXPECT_EQ(packet.payload()[i], bytes[i]);
    }
    EXPECT_EQ(packet.payload().size(), bytes.size());

    GstreamerBufferPacketTypeDescriptor gstreamer_buffer_packet_type_desc;
    EXPECT_TRUE(packet.header().type().type_descriptor().UnpackTo(
        &gstreamer_buffer_packet_type_desc));
    EXPECT_EQ(gstreamer_buffer_packet_type_desc.caps_string(), caps);
  }
}

TEST(PacketTest, PacketAsGstreamerBufferTest) {
  {
    std::string caps("video/x-raw");
    std::string bytes(10, 2);
    GstreamerBuffer src;
    src.set_caps_string(caps);
    src.assign(bytes);
    auto packet_status_or = MakePacket(src);
    EXPECT_TRUE(packet_status_or.ok());

    PacketAs<GstreamerBuffer> packet_as(
        std::move(packet_status_or).ValueOrDie());
    EXPECT_TRUE(packet_as.ok());
    PacketHeader header = packet_as.header();
    EXPECT_EQ(header.type().type_id(), PACKET_TYPE_GSTREAMER_BUFFER);

    GstreamerBuffer dst = std::move(packet_as).ValueOrDie();
    EXPECT_EQ(caps, dst.get_caps());
    EXPECT_EQ(bytes, std::string(dst.data(), dst.size()));
  }
}

TEST(PacketTest, MakePacketEosTest) {
  {
    std::string reason = "some reason";
    EosValue eos_value;
    eos_value.set_reason(reason);
    Eos eos(eos_value);
    auto packet_status_or = MakePacket(eos);
    EXPECT_TRUE(packet_status_or.ok());
    auto packet = std::move(packet_status_or).ValueOrDie();
    EXPECT_EQ(packet.header().type().type_id(), PACKET_TYPE_CONTROL_SIGNAL);
    EXPECT_FALSE(IsPacketFlagsSet(PacketFlags::kIsFrameHead, packet));
    EXPECT_FALSE(IsPacketFlagsSet(PacketFlags::kIsKeyFrame, packet));

    EosValue dst_eos_value;
    EXPECT_TRUE(dst_eos_value.ParseFromString(packet.payload()));
    EXPECT_EQ(eos_value.reason(), dst_eos_value.reason());
    EXPECT_EQ(dst_eos_value.reason(), reason);
  }
}

TEST(PacketTest, PacketAsEosTest) {
  {
    std::string reason = "some reason";
    Eos eos;
    eos.set_reason(reason);
    auto packet_status_or = MakePacket(eos);
    EXPECT_TRUE(packet_status_or.ok());

    PacketAs<Eos> packet_as(std::move(packet_status_or).ValueOrDie());
    EXPECT_TRUE(packet_as.ok());
    PacketHeader header = packet_as.header();
    EXPECT_EQ(header.type().type_id(), PACKET_TYPE_CONTROL_SIGNAL);

    eos = std::move(packet_as).ValueOrDie();
    EXPECT_EQ(eos.reason(), reason);
  }
  {
    std::string src("hey!");
    auto packet_status_or = MakePacket(src);
    PacketAs<Eos> packet_as(std::move(packet_status_or).ValueOrDie());
    EXPECT_FALSE(packet_as.ok());
  }
}

TEST(PacketTest, MakeEosPacketTest) {
  {
    std::string reason = "some reason";
    auto packet_status_or = MakeEosPacket(reason);
    EXPECT_TRUE(packet_status_or.ok());
    PacketAs<Eos> packet_as(std::move(packet_status_or).ValueOrDie());
    EXPECT_TRUE(packet_as.ok());
    PacketHeader header = packet_as.header();
    EXPECT_EQ(header.type().type_id(), PACKET_TYPE_CONTROL_SIGNAL);
    Eos eos = std::move(packet_as).ValueOrDie();
    EXPECT_EQ(eos.reason(), reason);
  }
}

}  // namespace aistreams
