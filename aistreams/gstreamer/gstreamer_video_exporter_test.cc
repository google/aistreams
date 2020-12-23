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

#include "aistreams/gstreamer/gstreamer_video_exporter.h"

#include <experimental/filesystem>
#include <string>

#include "aistreams/base/types/gstreamer_buffer.h"
#include "aistreams/gstreamer/type_utils.h"
#include "aistreams/port/gtest.h"
#include "aistreams/port/logging.h"
#include "aistreams/port/status.h"
#include "aistreams/port/status_macros.h"
#include "aistreams/port/statusor.h"
#include "aistreams/util/file_path.h"
#include "aistreams/util/producer_consumer_queue.h"
#include "aistreams/util/random_string.h"

namespace fs = std::experimental::filesystem;

namespace aistreams {

// TODO: Add a stream server source test. Requires making a mock video server.

TEST(GstreamerVideoExporter, GstreamerInputSourceBasicTest) {
  std::string output_dir = file::JoinPath(testing::TempDir(), RandomString(5));
  ASSERT_TRUE(fs::create_directory(output_dir));

  // Run the video exporter.
  GstreamerVideoExporter::Options options;
  options.max_frames_per_file = 10;
  options.output_dir = output_dir;
  options.upload_to_gcs = false;
  options.use_gstreamer_input_source = true;
  options.gstreamer_input_pipeline = "videotestsrc num-buffers=50 is-live=true";
  auto video_exporter = GstreamerVideoExporter::Create(options).ValueOrDie();
  auto status = video_exporter->Run();
  EXPECT_TRUE(status.ok());

  std::vector<std::string> output_video_paths;
  for (const auto& entry : fs::directory_iterator(output_dir)) {
    output_video_paths.push_back(entry.path());
  }
  EXPECT_EQ(output_video_paths.size(), 5);

  // Verify the results.
  {
    ProducerConsumerQueue<RawImage> pcqueue(20);
    GstreamerRunner::Options runner_options;
    runner_options.processing_pipeline_string = absl::StrFormat(
        "filesrc location=%s ! decodebin ! videoconvert ! videoscale ! "
        "video/x-raw,format=RGB,height=100,width=100",
        output_video_paths[0]);
    runner_options.receiver_callback =
        [&pcqueue](GstreamerBuffer buffer) -> Status {
      auto raw_image_status_or = ToRawImage(std::move(buffer));
      if (!raw_image_status_or.ok()) {
        LOG(ERROR) << raw_image_status_or.status();
      }
      pcqueue.Emplace(std::move(raw_image_status_or).ValueOrDie());
      return OkStatus();
    };
    auto runner_statusor = GstreamerRunner::Create(runner_options);
    ASSERT_TRUE(runner_statusor.ok());
    auto runner = std::move(runner_statusor).ValueOrDie();
    while (!runner->WaitUntilCompleted(absl::Seconds(1)))
      ;
    EXPECT_TRUE(runner->IsCompleted());

    // Interestingly, the gstreamer filesrc pipeline above swallows 1 frame.
    EXPECT_EQ(pcqueue.count() + 1, 10);
    RawImage raw_image;
    EXPECT_TRUE(pcqueue.TryPop(raw_image, absl::Seconds(1)));
    EXPECT_EQ(raw_image.height(), 100);
    EXPECT_EQ(raw_image.width(), 100);
    EXPECT_EQ(raw_image.channels(), 3);
  }

  ASSERT_TRUE(fs::remove_all(output_dir));
}

}  // namespace aistreams
