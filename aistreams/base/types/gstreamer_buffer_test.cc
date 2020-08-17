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

#include "aistreams/base/types/gstreamer_buffer.h"

#include <cstring>
#include <memory>
#include <string>

#include "aistreams/port/gtest.h"
#include "aistreams/port/logging.h"
#include "aistreams/port/status.h"
#include "aistreams/port/statusor.h"

namespace aistreams {

TEST(GstreamerBufferTest, DefaultBufferTest) {
  {
    GstreamerBuffer gstreamer_buffer;
    EXPECT_TRUE(gstreamer_buffer.get_caps().empty());
    EXPECT_EQ(gstreamer_buffer.get_caps_cstr()[0], '\0');
    std::string buffer = std::move(gstreamer_buffer).ReleaseBuffer();
    EXPECT_TRUE(buffer.empty());
  }
}

TEST(GstreamerBufferTest, CapsTest) {
  {
    std::string caps_string = "video/x-raw";
    GstreamerBuffer gstreamer_buffer;
    gstreamer_buffer.set_caps_string(caps_string);
    EXPECT_EQ(gstreamer_buffer.get_caps(), caps_string);
    EXPECT_FALSE(strcmp(gstreamer_buffer.get_caps_cstr(), caps_string.c_str()));
    const char another_caps_string[] = "video/x-h264";
    gstreamer_buffer.set_caps_string(another_caps_string);
    EXPECT_FALSE(strcmp(gstreamer_buffer.get_caps_cstr(), another_caps_string));
  }
}

TEST(GstreamerBufferTest, AssignTest) {
  {
    std::string some_data("hello");
    GstreamerBuffer gstreamer_buffer;
    gstreamer_buffer.assign(some_data.data(), some_data.size());
    std::string buffer_value = std::move(gstreamer_buffer).ReleaseBuffer();
    EXPECT_EQ(buffer_value, some_data);
  }
  {
    std::string some_data("hello");
    GstreamerBuffer gstreamer_buffer;
    gstreamer_buffer.assign(some_data);
    std::string buffer_value = std::move(gstreamer_buffer).ReleaseBuffer();
    EXPECT_EQ(buffer_value, some_data);
  }
  {
    std::string some_data("hello");
    GstreamerBuffer gstreamer_buffer;
    gstreamer_buffer.assign(std::string(some_data));
    std::string buffer_value = std::move(gstreamer_buffer).ReleaseBuffer();
    EXPECT_EQ(buffer_value, some_data);
  }
  {
    std::string some_data("hello");
    GstreamerBuffer gstreamer_buffer;
    gstreamer_buffer.assign(some_data);
    std::string buffer_value(gstreamer_buffer.data(), gstreamer_buffer.size());
    EXPECT_EQ(buffer_value, some_data);
  }
}

}  // namespace aistreams
