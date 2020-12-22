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

#include "absl/time/time.h"
#include "aistreams/base/types/gstreamer_buffer.h"
#include "aistreams/port/status.h"
#include "aistreams/port/statusor.h"

namespace aistreams {

// This class runs an arbitrary gstreamer pipeline.
//
// It also supports the ability to directly feed/fetch in-memory
// data to/from gstreamer.
//
// Please see the GstreamerRunner::Options for how to configure it.
class GstreamerRunner {
 public:
  using ReceiverCallback = std::function<Status(GstreamerBuffer)>;

  // Options for configuring the gstreamer runner.
  struct Options {
    // REQUIRED: The gstreamer pipeline string to run.
    //
    // This is the same string that you would normally feed to gst-launch:
    // gst-launch <processing-pipeline-string>
    std::string processing_pipeline_string;

    // OPTIONAL: If non-empty, an appsrc will be prepended to the processing
    // pipeline with caps set to this string.
    std::string appsrc_caps_string;

    // OPTIONAL: If non-empty, an appsink will be appended after the main
    // processing pipeline to deliver the result through the given callback.
    ReceiverCallback receiver_callback;

    // ----------------------------------------------
    // System configurations. Power users only.

    // Value of "sync" for appsink.
    bool appsink_sync = false;
  };

  // Create and run a gstreamer pipeline.
  static StatusOr<std::unique_ptr<GstreamerRunner>> Create(const Options&);

  // Feed a GstreamerBuffer object for processing.
  //
  // This is available only if you enable it in the Options.
  Status Feed(const GstreamerBuffer&) const;

  // Returns true if the pipeline has completed; otherwise, false.
  bool IsCompleted() const;

  // Blocks until the pipeline has completed or if the timeout expires.
  // Returns true if the pipeline has completed; otherwise, false.
  bool WaitUntilCompleted(absl::Duration timeout) const;

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
  class GstreamerRunnerImpl;
  std::unique_ptr<GstreamerRunnerImpl> gstreamer_runner_impl_;
};

}  // namespace aistreams

#endif  // AISTREAMS_GSTREAMER_GSTREAMER_RUNNER_H_
