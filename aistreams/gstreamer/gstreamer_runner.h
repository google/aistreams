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

#ifndef AISTREAMS_GSTREAMER_GSTREAMER_RUNNER_H_
#define AISTREAMS_GSTREAMER_GSTREAMER_RUNNER_H_

#include <atomic>
#include <functional>

#include "aistreams/base/types/gstreamer_buffer.h"
#include "aistreams/port/status.h"
#include "aistreams/port/statusor.h"

namespace aistreams {

// This class runs an arbitrary gstreamer pipeline.
//
// It also supports the ability to directly directly feed/fetch in-memory
// data to/from gstreamer.
class GstreamerRunner {
 public:
  using ReceiverCallback = std::function<Status(GstreamerBuffer)>;

  // Options for configuring the gstreamer runner.
  struct Options {
    // REQUIRED: The gstreamer pipeline string to run.
    //
    // It is the same string that you would normally feed to gst-launch:
    // gst-launch <processing-pipeline-string>
    std::string processing_pipeline_string;

    // REQUIRED: The caps of the appsrc prepended to the processing pipeline.
    std::string appsrc_caps_string;

    // OPTIONAL: If non-empty, an appsink will be appended after the main
    // processing pipeline to deliver the result through the given callback.
    ReceiverCallback receiver_callback;
  };
  static StatusOr<std::unique_ptr<GstreamerRunner>> Create(const Options&);

  // Feed a GstreamerBuffer object for processing.
  Status Feed(const GstreamerBuffer&);

  // Copy-control members.
  //
  // Please use the GstreamerRunner::Create rather than constructors directly.
  GstreamerRunner();
  ~GstreamerRunner();
  GstreamerRunner(const GstreamerRunner&) = delete;
  GstreamerRunner& operator=(const GstreamerRunner&) = delete;
  GstreamerRunner(GstreamerRunner&&) = delete;
  GstreamerRunner& operator=(GstreamerRunner&&) = delete;

 private:
  class GstreamerRuntimeImpl;
  std::unique_ptr<GstreamerRuntimeImpl> gstreamer_runtime_impl_;
};

}  // namespace aistreams

#endif  // AISTREAMS_GSTREAMER_GSTREAMER_RUNNER_H_
