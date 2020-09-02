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

#include "aistreams/c/ais_gstreamer_buffer.h"

#include <cstring>
#include <string>

#include "aistreams/c/ais_gstreamer_buffer_internal.h"
#include "aistreams/port/gtest.h"
#include "aistreams/port/logging.h"
#include "aistreams/port/status.h"
#include "aistreams/port/statusor.h"

namespace aistreams {

TEST(CAPI, AIS_GstreamerBufferTest) {
  AIS_GstreamerBuffer* ais_gstreamer_buffer = AIS_NewGstreamerBuffer();

  EXPECT_EQ(strcmp(AIS_GstreamerBufferGetCapsString(ais_gstreamer_buffer), ""),
            0);
  EXPECT_EQ(AIS_GstreamerBufferSize(ais_gstreamer_buffer), 0);

  AIS_GstreamerBufferSetCapsString("video/x-raw", ais_gstreamer_buffer);
  std::string src = "hello";
  AIS_GstreamerBufferAssign(src.c_str(), src.size(), ais_gstreamer_buffer);

  EXPECT_EQ(strcmp(AIS_GstreamerBufferGetCapsString(ais_gstreamer_buffer),
                   "video/x-raw"),
            0);
  EXPECT_EQ(AIS_GstreamerBufferSize(ais_gstreamer_buffer), src.size());
  std::string dst(ais_gstreamer_buffer->gstreamer_buffer.data(),
                  ais_gstreamer_buffer->gstreamer_buffer.size());
  EXPECT_EQ(src, dst);

  std::string dst2(AIS_GstreamerBufferSize(ais_gstreamer_buffer), '.');
  AIS_GstreamerBufferCopyTo(ais_gstreamer_buffer,
                            const_cast<char*>(dst2.data()));
  EXPECT_EQ(dst2, dst);

  AIS_DeleteGstreamerBuffer(ais_gstreamer_buffer);
}

}  // namespace aistreams
