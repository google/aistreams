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
// Objects of this class turns a sequence of packets from a stream into a
// sequence of video files.
//
// TODO: Add support for exporting from a local gstreamer pipeline.
// TODO: Add support for GCS upload (please coordinate with server guys).
class GstreamerVideoExporter {
 public:
  // Options to configure the GstreamerVideoExporter.
  struct Options {
    // ------------------------------------------------------------
    // Video writing configurations.

    // Maximum number of frames saved into each video file.
    // TODO: add max time interval and consider max file size.
    int max_frames_per_file = 200;

    // An optional prefix to the exported videos.
    std::string file_prefix;

    // ------------------------------------------------------------
    // Stream server packet source configuration.

    // Packet receiver configuration.
    ReceiverOptions receiver_options;

    // Timeout to receive a packet from a stream server.
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
