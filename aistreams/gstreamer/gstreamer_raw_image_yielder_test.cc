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

#include "aistreams/gstreamer/gstreamer_raw_image_yielder.h"

#include <gst/gst.h>

#include <string>

#include "aistreams/base/types/gstreamer_buffer.h"
#include "aistreams/base/types/raw_image.h"
#include "aistreams/gstreamer/gstreamer_raw_image_yielder.h"
#include "aistreams/port/canonical_errors.h"
#include "aistreams/port/gtest.h"
#include "aistreams/port/logging.h"
#include "aistreams/port/status.h"
#include "aistreams/port/status_macros.h"
#include "aistreams/port/statusor.h"
#include "aistreams/util/file_helpers.h"
#include "aistreams/util/producer_consumer_queue.h"

namespace aistreams {

namespace {

constexpr char kTestImageLenaPath[] = "testdata/jpegs/lena_color.jpg";
constexpr char kTestImageSquaresPath[] = "testdata/jpegs/squares_color.jpg";
constexpr char kTestImageGooglePath[] = "testdata/pngs/google_logo.png";
constexpr char kPngCapsString[] = "image/png";
constexpr char kJpegCapsString[] = "image/jpeg";
constexpr char kProcessingPipelineString[] =
    "decodebin ! videoconvert ! video/x-raw,format=RGB";

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

TEST(GstreamerRawImageYielder, JpegSequenceTest) {
  ProducerConsumerQueue<RawImage> pcqueue(10);

  auto callback = [&pcqueue](StatusOr<RawImage> raw_image_statusor) -> Status {
    if (!raw_image_statusor.ok()) {
      auto status = raw_image_statusor.status();
      if (status.code() == StatusCode::kNotFound) {
        pcqueue.TryEmplace(RawImage());
      } else {
        LOG(ERROR) << status;
      }
      return OkStatus();
    }
    pcqueue.TryEmplace(std::move(raw_image_statusor).ValueOrDie());
    return OkStatus();
  };

  GstreamerRawImageYielder::Options options;
  options.caps_string = kJpegCapsString;
  options.callback = callback;

  auto yielder_statusor = GstreamerRawImageYielder::Create(options);
  EXPECT_TRUE(yielder_statusor.ok());
  auto yielder = std::move(yielder_statusor).ValueOrDie();

  // Feed the jpeg.
  {
    GstreamerBuffer gstreamer_buffer =
        GstreamerBufferFromFile(kTestImageLenaPath, kJpegCapsString)
            .ValueOrDie();
    EXPECT_TRUE(yielder->Feed(gstreamer_buffer).ok());
  }

  // Feed a png. This should fail.
  {
    GstreamerBuffer gstreamer_buffer =
        GstreamerBufferFromFile(kTestImageGooglePath, kPngCapsString)
            .ValueOrDie();
    EXPECT_FALSE(yielder->Feed(gstreamer_buffer).ok());
  }

  // Feed the jpeg again.
  {
    GstreamerBuffer gstreamer_buffer =
        GstreamerBufferFromFile(kTestImageSquaresPath, kJpegCapsString)
            .ValueOrDie();
    EXPECT_TRUE(yielder->Feed(gstreamer_buffer).ok());
  }

  // Signal EOS.
  EXPECT_TRUE(yielder->SignalEOS().ok());

  // Feed the jpeg again. This should fail.
  {
    GstreamerBuffer gstreamer_buffer =
        GstreamerBufferFromFile(kTestImageSquaresPath, kJpegCapsString)
            .ValueOrDie();
    EXPECT_FALSE(yielder->Feed(gstreamer_buffer).ok());
  }

  // Verify results.
  EXPECT_EQ(pcqueue.count(), 3);
  {
    RawImage r;
    EXPECT_TRUE(pcqueue.TryPop(r, absl::Seconds(1)));
    EXPECT_EQ(r.format(), RAW_IMAGE_FORMAT_SRGB);
    EXPECT_EQ(r.height(), 512);
    EXPECT_EQ(r.width(), 512);
    EXPECT_EQ(r.channels(), 3);
    EXPECT_EQ(r.size(), 786432);
  }
  {
    RawImage r;
    EXPECT_TRUE(pcqueue.TryPop(r, absl::Seconds(1)));
    EXPECT_EQ(r.format(), RAW_IMAGE_FORMAT_SRGB);
    EXPECT_EQ(r.height(), 243);
    EXPECT_EQ(r.width(), 243);
    EXPECT_EQ(r.channels(), 3);
    EXPECT_EQ(r.size(), 177147);
  }
  {
    RawImage r;
    EXPECT_TRUE(pcqueue.TryPop(r, absl::Seconds(1)));
    EXPECT_EQ(r.format(), RAW_IMAGE_FORMAT_SRGB);
    EXPECT_EQ(r.height(), 0);
  }
}

TEST(GstreamerRawImageYielder, DtorSignalEOSTest) {
  ProducerConsumerQueue<RawImage> pcqueue(10);

  {
    auto callback =
        [&pcqueue](StatusOr<RawImage> raw_image_statusor) -> Status {
      if (!raw_image_statusor.ok()) {
        auto status = raw_image_statusor.status();
        if (status.code() == StatusCode::kNotFound) {
          pcqueue.TryEmplace(RawImage());
        } else {
          LOG(ERROR) << status;
        }
        return OkStatus();
      }
      pcqueue.TryEmplace(std::move(raw_image_statusor).ValueOrDie());
      return OkStatus();
    };

    GstreamerRawImageYielder::Options options;
    options.caps_string = kJpegCapsString;
    options.callback = callback;

    auto yielder_statusor = GstreamerRawImageYielder::Create(options);
    EXPECT_TRUE(yielder_statusor.ok());
    auto yielder = std::move(yielder_statusor).ValueOrDie();

    // Feed the jpeg.
    {
      GstreamerBuffer gstreamer_buffer =
          GstreamerBufferFromFile(kTestImageLenaPath, kJpegCapsString)
              .ValueOrDie();
      EXPECT_TRUE(yielder->Feed(gstreamer_buffer).ok());
    }
  }

  // Verify results.
  EXPECT_EQ(pcqueue.count(), 2);
  {
    RawImage r;
    EXPECT_TRUE(pcqueue.TryPop(r, absl::Seconds(1)));
    EXPECT_EQ(r.format(), RAW_IMAGE_FORMAT_SRGB);
    EXPECT_EQ(r.height(), 512);
    EXPECT_EQ(r.width(), 512);
    EXPECT_EQ(r.channels(), 3);
    EXPECT_EQ(r.size(), 786432);
  }
  {
    RawImage r;
    EXPECT_TRUE(pcqueue.TryPop(r, absl::Seconds(1)));
    EXPECT_EQ(r.format(), RAW_IMAGE_FORMAT_SRGB);
    EXPECT_EQ(r.height(), 0);
  }
}

}  // namespace aistreams
