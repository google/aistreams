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

namespace aistreams {

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

}  // namespace aistreams
