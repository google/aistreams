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

#include "aistreams/base/types/raw_image.h"

#include <memory>
#include <string>

#include "aistreams/base/types/raw_image_helpers.h"
#include "aistreams/port/gtest.h"
#include "aistreams/port/logging.h"
#include "aistreams/port/status.h"
#include "aistreams/port/statusor.h"
#include "aistreams/proto/types/raw_image.pb.h"

namespace aistreams {

TEST(RawImageHelpersTest, GetNumChannelsTest) {
  EXPECT_EQ(GetNumChannels(RAW_IMAGE_FORMAT_UNKNOWN), 1);
  EXPECT_EQ(GetNumChannels(RAW_IMAGE_FORMAT_SRGB), 3);
}

TEST(RawImageHelpersTest, GetBufferSizeTest) {
  {
    int height = 1080;
    int width = 1920;
    RawImageFormat format = RAW_IMAGE_FORMAT_SRGB;
    RawImageDescriptor desc;
    desc.set_format(format);
    desc.set_height(height);
    desc.set_width(width);
    auto bufsize = GetBufferSize(desc);
    EXPECT_TRUE(bufsize.ok());
    EXPECT_EQ(bufsize.ValueOrDie(), height * width * GetNumChannels(format));
  }
  {
    int height = 480;
    int width = 640;
    RawImageDescriptor desc;
    RawImageFormat format = RAW_IMAGE_FORMAT_UNKNOWN;
    desc.set_format(format);
    desc.set_height(height);
    desc.set_width(width);
    auto bufsize = GetBufferSize(desc);
    EXPECT_TRUE(bufsize.ok());
    EXPECT_EQ(bufsize.ValueOrDie(), height * width * GetNumChannels(format));
  }
  {
    int height = 1 << 16;
    int width = 1 << 16;
    RawImageFormat format = RAW_IMAGE_FORMAT_SRGB;
    RawImageDescriptor desc;
    desc.set_format(format);
    desc.set_height(height);
    desc.set_width(width);
    auto bufsize = GetBufferSize(desc);
    EXPECT_FALSE(bufsize.ok());
    LOG(ERROR) << bufsize.status();
  }
  {
    int height = 1 << 16;
    int width = 1 << 15;
    RawImageFormat format = RAW_IMAGE_FORMAT_SRGB;
    RawImageDescriptor desc;
    desc.set_format(format);
    desc.set_height(height);
    desc.set_width(width);
    auto bufsize = GetBufferSize(desc);
    EXPECT_FALSE(bufsize.ok());
    LOG(ERROR) << bufsize.status();
  }
}

TEST(RawImageHelpersTest, ValidateTest) {
  {
    int height = 1080;
    int width = 1920;
    RawImageDescriptor desc;
    desc.set_height(height);
    desc.set_width(width);
    auto status = Validate(desc);
    EXPECT_TRUE(status.ok());
  }
  {
    int height = -1;
    int width = 1920;
    RawImageFormat format = RAW_IMAGE_FORMAT_SRGB;
    RawImageDescriptor desc;
    desc.set_format(format);
    desc.set_height(height);
    desc.set_width(width);
    auto status = Validate(desc);
    EXPECT_FALSE(status.ok());
  }
  {
    int height = 1080;
    int width = -1;
    RawImageFormat format = RAW_IMAGE_FORMAT_SRGB;
    RawImageDescriptor desc;
    desc.set_format(format);
    desc.set_height(height);
    desc.set_width(width);
    auto status = Validate(desc);
    EXPECT_FALSE(status.ok());
  }
  {
    int height = 0;
    int width = 0;
    RawImageDescriptor desc;
    desc.set_height(height);
    desc.set_width(width);
    auto status = Validate(desc);
    EXPECT_TRUE(status.ok());
  }
}

TEST(RawImageTest, DefaultConstructorTest) {
  RawImage r;
  EXPECT_EQ(r.height(), 0);
  EXPECT_EQ(r.width(), 0);
  EXPECT_EQ(r.format(), RAW_IMAGE_FORMAT_SRGB);
  EXPECT_EQ(r.channels(), GetNumChannels(RAW_IMAGE_FORMAT_SRGB));
  EXPECT_EQ(r.size(), 0);
  EXPECT_NE(r.data(), nullptr);
}

TEST(RawImageTest, HeightWidthFormatConstructorTest) {
  {
    int height = 5;
    int width = 7;
    RawImageFormat format = RAW_IMAGE_FORMAT_SRGB;
    int channels = GetNumChannels(format);
    RawImage r(height, width, format);
    EXPECT_EQ(r.height(), height);
    EXPECT_EQ(r.width(), width);
    EXPECT_EQ(r.format(), format);
    EXPECT_EQ(r.channels(), channels);
    RawImageDescriptor desc;
    desc.set_format(format);
    desc.set_height(height);
    desc.set_width(width);
    auto bufsize = GetBufferSize(desc);
    EXPECT_TRUE(bufsize.ok());
    EXPECT_EQ(r.size(), bufsize.ValueOrDie());
  }
  {
    int height = -1;
    int width = 7;
    RawImageFormat format = RAW_IMAGE_FORMAT_SRGB;
    ASSERT_DEATH({ RawImage r(height, width, format); }, "");
  }
}

TEST(RawImageTest, RawImageDescriptorConstructorTest) {
  {
    int height = 5;
    int width = 7;
    RawImageFormat format = RAW_IMAGE_FORMAT_SRGB;
    int channels = GetNumChannels(format);
    RawImageDescriptor desc;
    desc.set_format(format);
    desc.set_height(height);
    desc.set_width(width);
    RawImage r(desc);
    EXPECT_EQ(r.height(), height);
    EXPECT_EQ(r.width(), width);
    EXPECT_EQ(r.format(), format);
    EXPECT_EQ(r.channels(), channels);
    auto bufsize = GetBufferSize(desc);
    EXPECT_TRUE(bufsize.ok());
    EXPECT_EQ(r.size(), bufsize.ValueOrDie());
  }
  {
    int height = 5;
    int width = -1;
    RawImageFormat format = RAW_IMAGE_FORMAT_SRGB;
    RawImageDescriptor desc;
    desc.set_format(format);
    desc.set_height(height);
    desc.set_width(width);
    ASSERT_DEATH({ RawImage r(desc); }, "");
  }
}

TEST(RawImageTest, DataAccessTest) {
  {
    int height = 1;
    int width = 1;
    RawImageFormat format = RAW_IMAGE_FORMAT_SRGB;
    RawImageDescriptor desc;
    desc.set_format(format);
    desc.set_height(height);
    desc.set_width(width);
    RawImage r(desc);
    r(0) = 0;
    r(1) = 1;
    r(2) = 2;
    for (size_t i = 0; i < r.size(); ++i) {
      EXPECT_EQ(r(i), i);
      EXPECT_EQ(r.data()[i], i);
    }
  }
}

TEST(RawImageTest, StringMoveConstructorTest) {
  {
    int height = 1;
    int width = 1;
    RawImageFormat format = RAW_IMAGE_FORMAT_SRGB;
    RawImageDescriptor desc;
    desc.set_format(format);
    desc.set_height(height);
    desc.set_width(width);
    std::string bytes(3, 0);
    bytes[0] = 0;
    bytes[1] = 1;
    bytes[2] = 2;
    RawImage r(desc, std::move(bytes));
    for (size_t i = 0; i < r.size(); ++i) {
      EXPECT_EQ(r(i), i);
    }
  }
  {
    int height = 2;
    int width = 2;
    RawImageFormat format = RAW_IMAGE_FORMAT_SRGB;
    RawImageDescriptor desc;
    desc.set_format(format);
    desc.set_height(height);
    desc.set_width(width);
    std::string bytes(1, 0);
    ASSERT_DEATH({ RawImage r(desc, std::move(bytes)); }, "");
  }
  {
    int height = 2;
    int width = 2;
    RawImageFormat format = RAW_IMAGE_FORMAT_SRGB;
    RawImageDescriptor desc;
    desc.set_format(format);
    desc.set_height(height);
    desc.set_width(width);
    std::string bytes(13, 0);
    ASSERT_DEATH({ RawImage r(desc, std::move(bytes)); }, "");
  }
}

TEST(RawImageTest, ReleaseBufferTest) {
  {
    int height = 2;
    int width = 2;
    RawImageFormat format = RAW_IMAGE_FORMAT_SRGB;
    RawImageDescriptor desc;
    desc.set_format(format);
    desc.set_height(height);
    desc.set_width(width);
    std::string src(12, 1);
    RawImage r(desc, std::move(src));
    std::string dst = std::move(r).ReleaseBuffer();
    EXPECT_EQ(dst.size(), 12);
    for (size_t i = 0; i < dst.size(); ++i) {
      EXPECT_EQ(dst[i], 1);
    }
  }
}

}  // namespace aistreams
