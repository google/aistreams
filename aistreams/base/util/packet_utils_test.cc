// Copyright 2020 Google Inc.

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

namespace aistreams {

TEST(PacketUtilsTest, IsEosTest) {
  {
    std::string reason = "some reason";
    auto packet_status_or = MakeEosPacket(reason);
    EXPECT_TRUE(packet_status_or.ok());
    auto packet = std::move(packet_status_or).ValueOrDie();
    EXPECT_TRUE(IsControlSignal(packet));
    EXPECT_TRUE(IsEos(packet));
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

}  // namespace aistreams
