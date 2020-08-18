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

#include "aistreams/gstreamer/gstreamer_runner.h"

#include <gst/gst.h>

#include <string>

#include "aistreams/base/types/gstreamer_buffer.h"
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
constexpr char kJpegCapsString[] = "image/jpeg";
constexpr char kPngCapsString[] = "image/png";
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

TEST(GstreamerRunner, JpegFeederTest) {
  ProducerConsumerQueue<GstreamerBuffer> pcqueue(10);

  // Options to process Jpeg images.
  GstreamerRunnerOptions options;
  options.processing_pipeline_string = kProcessingPipelineString;
  options.appsrc_caps_string = kJpegCapsString;

  // Allocate the runner and configure it.
  GstreamerRunner runner(options);
  Status status = runner.SetReceiver(
      [&pcqueue](GstreamerBuffer gstreamer_buffer) -> Status {
        pcqueue.TryEmplace(std::move(gstreamer_buffer));
        return OkStatus();
      });
  EXPECT_TRUE(status.ok());

  // Start the runner.
  EXPECT_TRUE(runner.Start().ok());

  // Feed the first jpeg.
  {
    GstreamerBuffer gstreamer_buffer =
        GstreamerBufferFromFile(kTestImageLenaPath, kJpegCapsString)
            .ValueOrDie();
    EXPECT_TRUE(runner.Feed(gstreamer_buffer).ok());
  }

  // Feed a different jpeg.
  {
    GstreamerBuffer gstreamer_buffer =
        GstreamerBufferFromFile(kTestImageSquaresPath, kJpegCapsString)
            .ValueOrDie();
    EXPECT_TRUE(runner.Feed(gstreamer_buffer).ok());
  }

  // Feed a png. This should fail.
  {
    GstreamerBuffer gstreamer_buffer =
        GstreamerBufferFromFile(kTestImageGooglePath, kPngCapsString)
            .ValueOrDie();
    EXPECT_FALSE(runner.Feed(gstreamer_buffer).ok());
  }

  // Feed the first jpeg again.
  {
    GstreamerBuffer gstreamer_buffer =
        GstreamerBufferFromFile(kTestImageLenaPath, kJpegCapsString)
            .ValueOrDie();
    EXPECT_TRUE(runner.Feed(gstreamer_buffer).ok());
  }

  // End the runner.
  EXPECT_TRUE(runner.End().ok());

  // Verify the results.
  {
    GstreamerBuffer gstreamer_buffer;
    EXPECT_TRUE(pcqueue.TryPop(gstreamer_buffer, absl::Seconds(1)));

    GstCaps* gst_caps = gst_caps_from_string(gstreamer_buffer.get_caps_cstr());
    GstStructure* structure = gst_caps_get_structure(gst_caps, 0);
    std::string media_type(gst_structure_get_name(structure));
    int height, width;
    EXPECT_TRUE(gst_structure_get_int(structure, "height", &height) == TRUE);
    EXPECT_TRUE(gst_structure_get_int(structure, "width", &width) == TRUE);
    std::string format(gst_structure_get_string(structure, "format"));
    gst_caps_unref(gst_caps);

    EXPECT_EQ(media_type, "video/x-raw");
    EXPECT_EQ(height, 512);
    EXPECT_EQ(width, 512);
    EXPECT_EQ(format, "RGB");
  }
  {
    GstreamerBuffer gstreamer_buffer;
    EXPECT_TRUE(pcqueue.TryPop(gstreamer_buffer, absl::Seconds(1)));

    GstCaps* gst_caps = gst_caps_from_string(gstreamer_buffer.get_caps_cstr());
    GstStructure* structure = gst_caps_get_structure(gst_caps, 0);
    std::string media_type(gst_structure_get_name(structure));
    int height, width;
    EXPECT_TRUE(gst_structure_get_int(structure, "height", &height) == TRUE);
    EXPECT_TRUE(gst_structure_get_int(structure, "width", &width) == TRUE);
    std::string format(gst_structure_get_string(structure, "format"));
    gst_caps_unref(gst_caps);

    EXPECT_EQ(media_type, "video/x-raw");
    EXPECT_EQ(height, 243);
    EXPECT_EQ(width, 243);
    EXPECT_EQ(format, "RGB");
  }
  {
    GstreamerBuffer gstreamer_buffer;
    EXPECT_TRUE(pcqueue.TryPop(gstreamer_buffer, absl::Seconds(1)));

    GstCaps* gst_caps = gst_caps_from_string(gstreamer_buffer.get_caps_cstr());
    GstStructure* structure = gst_caps_get_structure(gst_caps, 0);
    std::string media_type(gst_structure_get_name(structure));
    int height, width;
    EXPECT_TRUE(gst_structure_get_int(structure, "height", &height) == TRUE);
    EXPECT_TRUE(gst_structure_get_int(structure, "width", &width) == TRUE);
    std::string format(gst_structure_get_string(structure, "format"));
    gst_caps_unref(gst_caps);

    EXPECT_EQ(media_type, "video/x-raw");
    EXPECT_EQ(height, 512);
    EXPECT_EQ(width, 512);
    EXPECT_EQ(format, "RGB");
  }
  {
    GstreamerBuffer gstreamer_buffer;
    EXPECT_FALSE(pcqueue.TryPop(gstreamer_buffer, absl::Seconds(1)));
  }
}

TEST(GstreamerRunner, PipelineChangeTest) {
  ProducerConsumerQueue<GstreamerBuffer> pcqueue(10);

  // Setup a Jpeg pipeline.
  GstreamerRunnerOptions options;
  options.processing_pipeline_string = kProcessingPipelineString;
  options.appsrc_caps_string = kJpegCapsString;

  GstreamerRunner runner(options);
  Status status = runner.SetReceiver(
      [&pcqueue](GstreamerBuffer gstreamer_buffer) -> Status {
        pcqueue.TryEmplace(std::move(gstreamer_buffer));
        return OkStatus();
      });
  EXPECT_TRUE(status.ok());

  // Start the jpeg runner.
  EXPECT_TRUE(runner.Start().ok());

  // Feed a jpeg.
  {
    GstreamerBuffer gstreamer_buffer =
        GstreamerBufferFromFile(kTestImageLenaPath, kJpegCapsString)
            .ValueOrDie();
    EXPECT_TRUE(runner.Feed(gstreamer_buffer).ok());
  }

  // Feed a png. This should fail.
  {
    GstreamerBuffer gstreamer_buffer =
        GstreamerBufferFromFile(kTestImageGooglePath, kPngCapsString)
            .ValueOrDie();
    EXPECT_FALSE(runner.Feed(gstreamer_buffer).ok());
  }

  // End the jpeg runner.
  EXPECT_TRUE(runner.End().ok());

  // Setup a Png pipeline.
  options.processing_pipeline_string = kProcessingPipelineString;
  options.appsrc_caps_string = kPngCapsString;
  runner.SetOptions(options);

  status = runner.SetReceiver(
      [&pcqueue](GstreamerBuffer gstreamer_buffer) -> Status {
        pcqueue.TryEmplace(std::move(gstreamer_buffer));
        return OkStatus();
      });
  EXPECT_TRUE(status.ok());

  // Start the png runner.
  EXPECT_TRUE(runner.Start().ok());

  // Feed a jpeg. This should fail.
  {
    GstreamerBuffer gstreamer_buffer =
        GstreamerBufferFromFile(kTestImageLenaPath, kJpegCapsString)
            .ValueOrDie();
    EXPECT_FALSE(runner.Feed(gstreamer_buffer).ok());
  }

  // Feed a png.
  {
    GstreamerBuffer gstreamer_buffer =
        GstreamerBufferFromFile(kTestImageGooglePath, kPngCapsString)
            .ValueOrDie();
    EXPECT_TRUE(runner.Feed(gstreamer_buffer).ok());
  }

  // End the png runner.
  EXPECT_TRUE(runner.End().ok());

  // Verify the results.
  {
    GstreamerBuffer gstreamer_buffer;
    EXPECT_TRUE(pcqueue.TryPop(gstreamer_buffer, absl::Seconds(1)));

    GstCaps* gst_caps = gst_caps_from_string(gstreamer_buffer.get_caps_cstr());
    GstStructure* structure = gst_caps_get_structure(gst_caps, 0);
    std::string media_type(gst_structure_get_name(structure));
    int height, width;
    EXPECT_TRUE(gst_structure_get_int(structure, "height", &height) == TRUE);
    EXPECT_TRUE(gst_structure_get_int(structure, "width", &width) == TRUE);
    std::string format(gst_structure_get_string(structure, "format"));
    gst_caps_unref(gst_caps);

    EXPECT_EQ(media_type, "video/x-raw");
    EXPECT_EQ(height, 512);
    EXPECT_EQ(width, 512);
    EXPECT_EQ(format, "RGB");
  }
  {
    GstreamerBuffer gstreamer_buffer;
    EXPECT_TRUE(pcqueue.TryPop(gstreamer_buffer, absl::Seconds(1)));

    GstCaps* gst_caps = gst_caps_from_string(gstreamer_buffer.get_caps_cstr());
    GstStructure* structure = gst_caps_get_structure(gst_caps, 0);
    std::string media_type(gst_structure_get_name(structure));
    int height, width;
    EXPECT_TRUE(gst_structure_get_int(structure, "height", &height) == TRUE);
    EXPECT_TRUE(gst_structure_get_int(structure, "width", &width) == TRUE);
    std::string format(gst_structure_get_string(structure, "format"));
    gst_caps_unref(gst_caps);

    EXPECT_EQ(media_type, "video/x-raw");
    EXPECT_EQ(height, 225);
    EXPECT_EQ(width, 225);
    EXPECT_EQ(format, "RGB");
  }
  {
    GstreamerBuffer gstreamer_buffer;
    EXPECT_FALSE(pcqueue.TryPop(gstreamer_buffer, absl::Seconds(1)));
  }
}

}  // namespace aistreams
