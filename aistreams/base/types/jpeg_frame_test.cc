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

#include "aistreams/base/types/jpeg_frame.h"

#include <memory>
#include <string>

#include "aistreams/port/gtest.h"
#include "aistreams/port/logging.h"
#include "aistreams/port/status.h"
#include "aistreams/port/statusor.h"

namespace aistreams {

TEST(JpegFrameTest, StringConstructorTest) {
  {
    std::string bytes(10, 1);
    JpegFrame jpg(std::move(bytes));
    EXPECT_NE(jpg.data(), nullptr);
    EXPECT_EQ(jpg.size(), 10);
    for (size_t i = 0; i < jpg.size(); ++i) {
      EXPECT_EQ(jpg.data()[i], 1);
    }
  }
}

}  // namespace aistreams
