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

#ifndef AISTREAMS_GSTREAMER_GSTREAMER_VIDEO_WRITER_H_
#define AISTREAMS_GSTREAMER_GSTREAMER_VIDEO_WRITER_H_

#include <functional>
#include <memory>

#include "aistreams/base/types/gstreamer_buffer.h"
#include "aistreams/base/types/raw_image.h"
#include "aistreams/gstreamer/gstreamer_runner.h"
#include "aistreams/port/status.h"
#include "aistreams/port/statusor.h"

namespace aistreams {

// This class writes a single video file from frames it receives.
class GstreamerVideoWriter {
 public:
  // Options to configure the GstreamerVideoWriter.
  struct Options {
    // The path to the output video file.
    std::string file_path;

    // The caps string of all gstreamer buffers that would be fed.
    std::string caps_string;
  };

  // Create an instance in a fully initialized state.
  static StatusOr<std::unique_ptr<GstreamerVideoWriter>> Create(const Options&);

  // Add a gstreamer buffer into the output video.
  //
  // It must have the same caps as that specified in Options.
  Status Put(const GstreamerBuffer&);

  // Copy-control members. Use Create() rather than the constructors.
  explicit GstreamerVideoWriter(const Options&);
  ~GstreamerVideoWriter() = default;
  GstreamerVideoWriter() = delete;
  GstreamerVideoWriter(const GstreamerVideoWriter&) = delete;
  GstreamerVideoWriter& operator=(const GstreamerVideoWriter&) = delete;
  GstreamerVideoWriter(GstreamerVideoWriter&&) = delete;
  GstreamerVideoWriter& operator=(GstreamerVideoWriter&&) = delete;

 private:
  Status Initialize();

  Options options_;
  std::unique_ptr<GstreamerRunner> gstreamer_runner_ = nullptr;
};

}  // namespace aistreams

#endif  // AISTREAMS_GSTREAMER_GSTREAMER_VIDEO_WRITER_H_
