/*
 * Copyright 2020 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef AISTREAMS_GSTREAMER_GSTREAMER_VIDEO_EXPORTER_H_
#define AISTREAMS_GSTREAMER_GSTREAMER_VIDEO_EXPORTER_H_

#include <functional>
#include <memory>
#include <thread>

#include "absl/synchronization/mutex.h"
#include "aistreams/base/types/gstreamer_buffer.h"
#include "aistreams/base/types/raw_image.h"
#include "aistreams/base/wrappers/receivers.h"
#include "aistreams/gstreamer/gstreamer_raw_image_yielder.h"
#include "aistreams/port/status.h"
#include "aistreams/port/statusor.h"
#include "aistreams/util/completion_signal.h"
#include "aistreams/util/producer_consumer_queue.h"

namespace aistreams {

// This class exports a sequence of video files.
//
// Objects of this class accepts sequence of packets from a stream or raw image
// outputs of a gstreamer pipeline and turns them into a sequence of video
// files. They can then be optionally uploaded to a GCS bucket.
class GstreamerVideoExporter {
 public:
  // Options to configure the GstreamerVideoExporter.
  struct Options {
    // ------------------------------------------------------------
    // Video writing configurations.

    // Maximum number of frames saved into each video file.
    // TODO: add max time interval and consider max file size.
    int max_frames_per_file = 200;

    // The directory into which video files are saved.
    //
    // Leaving this empty will save to the current working directory.
    std::string output_dir;

    // An optional prefix to the exported videos.
    //
    // This is for specifying a file prefix, not an output directory. Use
    // `output_dir` to change the destination output directory.
    std::string file_prefix;

    // ------------------------------------------------------------
    // GCS uploading configurations.

    // Whether to upload to GCS.
    bool upload_to_gcs = false;

    // Whether to keep a local copy of the uploaded video.
    bool keep_local = true;

    // The GCS bucket to upload into.
    std::string gcs_bucket_name;

    // The sub-directory of the GCS bucket to upload into.
    //
    // Leaving this empty will write to the top level of the bucket.
    std::string gcs_object_dir;

    // ------------------------------------------------------------
    // Video source configuration.

    // If true, uses a gstreamer pipeline as the video input source.
    // Otherwise, use a stream server source.
    bool use_gstreamer_input_source = false;

    // The gstreamer pipeline used as the video input source.
    //
    // This pipeline must produce raw images; i.e. the final element should
    // yield the buffers with caps video/x-raw.
    std::string gstreamer_input_pipeline;

    // Stream server source's receiver configuration.
    ReceiverOptions receiver_options;

    // Stream server source's timeout to receive packets.
    absl::Duration receiver_timeout = absl::Seconds(10);

    // ------------------------------------------------------------
    // System configurations.

    // Size of internal work buffers.
    int working_buffer_size = 100;

    // The deadline for workers to gracefully finalize their work.
    absl::Duration finalization_deadline = absl::Seconds(5);
  };

  // Create a fully initialized video exporter.
  static StatusOr<std::unique_ptr<GstreamerVideoExporter>> Create(
      const Options&);

  // Blocking call to run the video exporter to completion.
  //
  // Each instance of this class may call this at most once. You should create a
  // new instance if another call is needed.
  Status Run();

  // Copy-control members. Use Create() rather than the constructors.
  ~GstreamerVideoExporter();
  GstreamerVideoExporter(const Options&);
  GstreamerVideoExporter(const GstreamerVideoExporter&) = delete;
  GstreamerVideoExporter& operator=(const GstreamerVideoExporter&) = delete;
  GstreamerVideoExporter(GstreamerVideoExporter&&) = delete;
  GstreamerVideoExporter& operator=(GstreamerVideoExporter&&) = delete;

 private:
  Options options_;
  bool has_been_run_ = false;
};

}  // namespace aistreams

#endif  // AISTREAMS_GSTREAMER_GSTREAMER_VIDEO_EXPORTER_H_
