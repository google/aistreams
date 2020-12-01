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

#include "aistreams/gstreamer/type_utils.h"

#include <string>

#include "aistreams/base/packet.h"
#include "aistreams/base/types/gstreamer_buffer.h"
#include "aistreams/base/types/raw_image.h"
#include "aistreams/gstreamer/gstreamer_runner.h"
#include "aistreams/port/canonical_errors.h"
#include "aistreams/port/gtest.h"
#include "aistreams/port/logging.h"
#include "aistreams/port/status.h"
#include "aistreams/port/status_macros.h"
#include "aistreams/port/statusor.h"
#include "aistreams/proto/types/raw_image.pb.h"
#include "aistreams/util/file_helpers.h"
#include "aistreams/util/producer_consumer_queue.h"

namespace aistreams {

namespace {

constexpr char kTestImageLenaPath[] = "testdata/jpegs/lena_color.jpg";
constexpr char kTestImageSquaresPath[] = "testdata/jpegs/squares_color.jpg";
constexpr char kJpegCapsString[] = "image/jpeg";
constexpr char kRgbPipeline[] =
    "decodebin ! videoconvert ! video/x-raw,format=RGB";
constexpr char kYuvPipeline[] = "decodebin";
constexpr char kRgbaPipeline[] =
    "decodebin ! videoconvert ! video/x-raw,format=RGBA";

StatusOr<GstreamerBuffer> GstreamerBufferFromFile(
    const std::string& fname, const std::string& caps_string) {
  std::string file_contents;
  AIS_RETURN_IF_ERROR(file::GetContents(fname, &file_contents));
  GstreamerBuffer gstreamer_buffer;
  gstreamer_buffer.set_caps_string(caps_string);
  gstreamer_buffer.assign(std::move(file_contents));
  return gstreamer_buffer;
}

}  // namespace

TEST(TypeUtils, NoPaddingTest) {
  ProducerConsumerQueue<GstreamerBuffer> pcqueue(1);

  {
    // Setup a pipeline to convert a jpeg into an RGB image.
    GstreamerRunner::Options options;
    options.processing_pipeline_string = kRgbPipeline;
    options.appsrc_caps_string = kJpegCapsString;
    options.receiver_callback =
        [&pcqueue](GstreamerBuffer gstreamer_buffer) -> Status {
      pcqueue.TryEmplace(std::move(gstreamer_buffer));
      return OkStatus();
    };
    auto runner_statusor = GstreamerRunner::Create(options);
    EXPECT_TRUE(runner_statusor.ok());
    auto runner = std::move(runner_statusor).ValueOrDie();

    // Decode the jpeg into an RGB image and close the runner.
    GstreamerBuffer gstreamer_buffer =
        GstreamerBufferFromFile(kTestImageLenaPath, kJpegCapsString)
            .ValueOrDie();
    EXPECT_TRUE(runner->Feed(gstreamer_buffer).ok());
  }

  {
    // Get the decoded image and convert it to an RawImage.
    GstreamerBuffer gstreamer_buffer;
    EXPECT_TRUE(pcqueue.TryPop(gstreamer_buffer, absl::Seconds(1)));
    auto raw_image_statusor = ToRawImage(std::move(gstreamer_buffer));
    EXPECT_TRUE(raw_image_statusor.ok());
    RawImage r = std::move(raw_image_statusor).ValueOrDie();
    EXPECT_EQ(r.format(), RAW_IMAGE_FORMAT_SRGB);
    EXPECT_EQ(r.height(), 512);
    EXPECT_EQ(r.width(), 512);
    EXPECT_EQ(r.channels(), 3);
    EXPECT_EQ(r.size(), 786432);
  }
}

TEST(TypeUtils, PaddingTest) {
  ProducerConsumerQueue<GstreamerBuffer> pcqueue(1);

  {
    // Setup a pipeline to convert a jpeg into an RGB image.
    GstreamerRunner::Options options;
    options.processing_pipeline_string = kRgbPipeline;
    options.appsrc_caps_string = kJpegCapsString;
    options.receiver_callback =
        [&pcqueue](GstreamerBuffer gstreamer_buffer) -> Status {
      pcqueue.TryEmplace(std::move(gstreamer_buffer));
      return OkStatus();
    };
    auto runner_statusor = GstreamerRunner::Create(options);
    EXPECT_TRUE(runner_statusor.ok());
    auto runner = std::move(runner_statusor).ValueOrDie();

    // Decode the jpeg into an RGB image and close the runner.
    GstreamerBuffer gstreamer_buffer =
        GstreamerBufferFromFile(kTestImageSquaresPath, kJpegCapsString)
            .ValueOrDie();
    EXPECT_TRUE(runner->Feed(gstreamer_buffer).ok());
  }

  {
    // Get the decoded image and convert it to an RawImage.
    GstreamerBuffer gstreamer_buffer;
    EXPECT_TRUE(pcqueue.TryPop(gstreamer_buffer, absl::Seconds(1)));
    EXPECT_EQ(gstreamer_buffer.size(), 177876);
    auto raw_image_statusor = ToRawImage(std::move(gstreamer_buffer));
    EXPECT_TRUE(raw_image_statusor.ok());
    RawImage r = std::move(raw_image_statusor).ValueOrDie();
    EXPECT_EQ(r.format(), RAW_IMAGE_FORMAT_SRGB);
    EXPECT_EQ(r.height(), 243);
    EXPECT_EQ(r.width(), 243);
    EXPECT_EQ(r.channels(), 3);
    EXPECT_EQ(r.size(), 177147);
  }
}

TEST(TypeUtils, YuvFailTest) {
  ProducerConsumerQueue<GstreamerBuffer> pcqueue(1);

  {
    // Setup a pipeline to convert a jpeg into an RGB image.
    GstreamerRunner::Options options;
    options.processing_pipeline_string = kYuvPipeline;
    options.appsrc_caps_string = kJpegCapsString;
    options.receiver_callback =
        [&pcqueue](GstreamerBuffer gstreamer_buffer) -> Status {
      pcqueue.TryEmplace(std::move(gstreamer_buffer));
      return OkStatus();
    };
    auto runner_statusor = GstreamerRunner::Create(options);
    EXPECT_TRUE(runner_statusor.ok());
    auto runner = std::move(runner_statusor).ValueOrDie();

    // Decode the jpeg into an RGB image and close the runner.
    GstreamerBuffer gstreamer_buffer =
        GstreamerBufferFromFile(kTestImageSquaresPath, kJpegCapsString)
            .ValueOrDie();
    EXPECT_TRUE(runner->Feed(gstreamer_buffer).ok());
  }

  {
    // Get the decoded image and convert it to an RawImage.
    GstreamerBuffer gstreamer_buffer;
    EXPECT_TRUE(pcqueue.TryPop(gstreamer_buffer, absl::Seconds(1)));
    auto raw_image_statusor = ToRawImage(std::move(gstreamer_buffer));
    EXPECT_FALSE(raw_image_statusor.ok());
    LOG(ERROR) << raw_image_statusor.status();
  }
}

TEST(TypeUtils, RgbaFailTest) {
  ProducerConsumerQueue<GstreamerBuffer> pcqueue(1);

  {
    // Setup a pipeline to convert a jpeg into an RGB image.
    GstreamerRunner::Options options;
    options.processing_pipeline_string = kRgbaPipeline;
    options.appsrc_caps_string = kJpegCapsString;
    options.receiver_callback =
        [&pcqueue](GstreamerBuffer gstreamer_buffer) -> Status {
      pcqueue.TryEmplace(std::move(gstreamer_buffer));
      return OkStatus();
    };
    auto runner_statusor = GstreamerRunner::Create(options);
    EXPECT_TRUE(runner_statusor.ok());
    auto runner = std::move(runner_statusor).ValueOrDie();

    // Decode the jpeg into an RGB image and close the runner.
    GstreamerBuffer gstreamer_buffer =
        GstreamerBufferFromFile(kTestImageSquaresPath, kJpegCapsString)
            .ValueOrDie();
    EXPECT_TRUE(runner->Feed(gstreamer_buffer).ok());
  }

  {
    // Get the decoded image and convert it to an RawImage.
    GstreamerBuffer gstreamer_buffer;
    EXPECT_TRUE(pcqueue.TryPop(gstreamer_buffer, absl::Seconds(1)));
    auto raw_image_statusor = ToRawImage(std::move(gstreamer_buffer));
    EXPECT_FALSE(raw_image_statusor.ok());
    LOG(ERROR) << raw_image_statusor.status();
  }
}

TEST(TypeUtils, GstreamerBufferPacketToGstreamerBuffer) {
  {
    GstreamerBuffer gstreamer_buffer_src =
        GstreamerBufferFromFile(kTestImageLenaPath, kJpegCapsString)
            .ValueOrDie();
    auto packet = MakePacket(gstreamer_buffer_src).ValueOrDie();
    auto gstreamer_buffer_statusor = ToGstreamerBuffer(std::move(packet));
    EXPECT_TRUE(gstreamer_buffer_statusor.ok());
    auto gstreamer_buffer_dst =
        std::move(gstreamer_buffer_statusor).ValueOrDie();
    EXPECT_EQ(gstreamer_buffer_src.get_caps(), gstreamer_buffer_dst.get_caps());
    std::string data_src = std::move(gstreamer_buffer_src).ReleaseBuffer();
    std::string data_dst = std::move(gstreamer_buffer_dst).ReleaseBuffer();
    EXPECT_EQ(data_src, data_dst);
  }
}

TEST(TypeUtils, JpegFramePacketToGstreamerBuffer) {
  {
    GstreamerBuffer gstreamer_buffer_src =
        GstreamerBufferFromFile(kTestImageLenaPath, kJpegCapsString)
            .ValueOrDie();
    GstreamerBuffer gstreamer_buffer_tmp(gstreamer_buffer_src);
    JpegFrame jpeg_frame(std::move(gstreamer_buffer_tmp).ReleaseBuffer());
    auto packet = MakePacket(jpeg_frame).ValueOrDie();
    auto gstreamer_buffer_statusor = ToGstreamerBuffer(std::move(packet));
    EXPECT_TRUE(gstreamer_buffer_statusor.ok());
    auto gstreamer_buffer_dst =
        std::move(gstreamer_buffer_statusor).ValueOrDie();
    EXPECT_EQ(gstreamer_buffer_src.get_caps(), gstreamer_buffer_dst.get_caps());
    std::string data_src = std::move(gstreamer_buffer_src).ReleaseBuffer();
    std::string data_dst = std::move(gstreamer_buffer_dst).ReleaseBuffer();
    EXPECT_EQ(data_src, data_dst);
  }
}

TEST(TypeUtils, NoPaddingRgbRawImagePacketToGstreamerBuffer) {
  // Get a gstreamer raw image by decoding a JPEG.
  ProducerConsumerQueue<GstreamerBuffer> pcqueue(1);
  {
    // Setup a pipeline to convert a jpeg into an RGB image.
    GstreamerRunner::Options options;
    options.processing_pipeline_string = kRgbPipeline;
    options.appsrc_caps_string = kJpegCapsString;
    options.receiver_callback =
        [&pcqueue](GstreamerBuffer gstreamer_buffer) -> Status {
      pcqueue.TryEmplace(std::move(gstreamer_buffer));
      return OkStatus();
    };
    auto runner_statusor = GstreamerRunner::Create(options);
    EXPECT_TRUE(runner_statusor.ok());
    auto runner = std::move(runner_statusor).ValueOrDie();

    // Decode the jpeg into an RGB image and close the runner.
    GstreamerBuffer gstreamer_buffer =
        GstreamerBufferFromFile(kTestImageLenaPath, kJpegCapsString)
            .ValueOrDie();
    EXPECT_TRUE(runner->Feed(gstreamer_buffer).ok());
  }

  // Actual test starts here.
  {
    GstreamerBuffer gstreamer_buffer_src;
    EXPECT_TRUE(pcqueue.TryPop(gstreamer_buffer_src, absl::Seconds(1)));
    auto raw_image_statusor = ToRawImage(gstreamer_buffer_src);
    EXPECT_TRUE(raw_image_statusor.ok());
    RawImage r_src = std::move(raw_image_statusor).ValueOrDie();
    EXPECT_EQ(r_src.format(), RAW_IMAGE_FORMAT_SRGB);
    EXPECT_EQ(r_src.height(), 512);
    EXPECT_EQ(r_src.width(), 512);
    EXPECT_EQ(r_src.channels(), 3);
    EXPECT_EQ(r_src.size(), 786432);

    auto packet = MakePacket(r_src).ValueOrDie();
    auto gstreamer_buffer_statusor = ToGstreamerBuffer(std::move(packet));
    EXPECT_TRUE(gstreamer_buffer_statusor.ok());
    auto gstreamer_buffer_dst =
        std::move(gstreamer_buffer_statusor).ValueOrDie();
    raw_image_statusor = ToRawImage(std::move(gstreamer_buffer_dst));
    EXPECT_TRUE(raw_image_statusor.ok());
    RawImage r_dst = std::move(raw_image_statusor).ValueOrDie();
    EXPECT_EQ(r_dst.format(), r_src.format());
    EXPECT_EQ(r_dst.height(), r_src.height());
    EXPECT_EQ(r_dst.width(), r_src.width());
    EXPECT_EQ(r_dst.channels(), r_src.channels());
    EXPECT_EQ(r_dst.size(), r_src.size());
  }
}

TEST(TypeUtils, PaddingRgbRawImagePacketToGstreamerBuffer) {
  // Get a gstreamer raw image by decoding a JPEG.
  ProducerConsumerQueue<GstreamerBuffer> pcqueue(1);
  {
    // Setup a pipeline to convert a jpeg into an RGB image.
    GstreamerRunner::Options options;
    options.processing_pipeline_string = kRgbPipeline;
    options.appsrc_caps_string = kJpegCapsString;
    options.receiver_callback =
        [&pcqueue](GstreamerBuffer gstreamer_buffer) -> Status {
      pcqueue.TryEmplace(std::move(gstreamer_buffer));
      return OkStatus();
    };
    auto runner_statusor = GstreamerRunner::Create(options);
    EXPECT_TRUE(runner_statusor.ok());
    auto runner = std::move(runner_statusor).ValueOrDie();

    // Decode the jpeg into an RGB image and close the runner.
    GstreamerBuffer gstreamer_buffer =
        GstreamerBufferFromFile(kTestImageSquaresPath, kJpegCapsString)
            .ValueOrDie();
    EXPECT_TRUE(runner->Feed(gstreamer_buffer).ok());
  }

  // Actual test starts here.
  {
    GstreamerBuffer gstreamer_buffer_src;
    EXPECT_TRUE(pcqueue.TryPop(gstreamer_buffer_src, absl::Seconds(1)));
    auto raw_image_statusor = ToRawImage(gstreamer_buffer_src);
    EXPECT_TRUE(raw_image_statusor.ok());
    RawImage r_src = std::move(raw_image_statusor).ValueOrDie();
    EXPECT_EQ(r_src.format(), RAW_IMAGE_FORMAT_SRGB);
    EXPECT_EQ(r_src.height(), 243);
    EXPECT_EQ(r_src.width(), 243);
    EXPECT_EQ(r_src.channels(), 3);
    EXPECT_EQ(r_src.size(), 177147);

    auto packet = MakePacket(r_src).ValueOrDie();
    auto gstreamer_buffer_statusor = ToGstreamerBuffer(std::move(packet));
    EXPECT_TRUE(gstreamer_buffer_statusor.ok());
    auto gstreamer_buffer_dst =
        std::move(gstreamer_buffer_statusor).ValueOrDie();
    EXPECT_EQ(gstreamer_buffer_dst.size(), 177876);
    raw_image_statusor = ToRawImage(std::move(gstreamer_buffer_dst));
    EXPECT_TRUE(raw_image_statusor.ok());
    RawImage r_dst = std::move(raw_image_statusor).ValueOrDie();
    EXPECT_EQ(r_dst.format(), r_src.format());
    EXPECT_EQ(r_dst.height(), r_src.height());
    EXPECT_EQ(r_dst.width(), r_src.width());
    EXPECT_EQ(r_dst.channels(), r_src.channels());
    EXPECT_EQ(r_dst.size(), r_src.size());
  }
}

TEST(TypeUtils, EosPacketToGstreamerBufferFail) {
  {
    auto packet = MakeEosPacket("no reason").ValueOrDie();
    auto gstreamer_buffer_statusor = ToGstreamerBuffer(packet);
    EXPECT_FALSE(gstreamer_buffer_statusor.ok());
  }
}

}  // namespace aistreams
