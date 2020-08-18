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

// Options for configuring the gstreamer runner.
struct GstreamerRunnerOptions {
  // A string that specifies the processing portion of a gstreamer pipeline.
  //
  // More specifically, it can be any string that forms a valid gstreamer
  // pipeline when it substitutes <processing-pipeline-string> in the following
  // gst-launch command:
  //
  // gst-launch appsrc ! <processing-pipeline-string> ! appsink
  std::string processing_pipeline_string;

  // The string representing the appsrc caps that you intend to feed.
  std::string appsrc_caps_string;
};

// This class manages a running gstreamer pipeline and supports an interface to
// directly input/output in-memory data to/from gstreamer.
//
// The runner is either in the "Started" or "Ended" state.
// It begins its lifetime (constructed) in the "Ended" state.
// It may transition to the "Started" state by calling Start().
// Similarly, it may transition to the "Ended" state by calling End().
//
// You may only set options and callbacks when the runner is "Ended".
// You may only feed and receive buffers when the runner has "Started".
//
// Buffers fed will have their results be completed and delivered in FIFO order.
// When the runner transitions to "Ended", any processing that are currently
// in-flight and any un-delivered results will be discarded.
//
// Most resources are allocated and initialized when Start() is called. To
// properly cleanup, you must also call End(). Merely running the destructor
// will leak resources.
class GstreamerRunner {
 public:
  using ReceiverCallback = std::function<Status(GstreamerBuffer)>;

  // Constructors only set the options.
  //
  // The default constructor default initializes GstreamerRunnerOptions.
  GstreamerRunner();
  explicit GstreamerRunner(const GstreamerRunnerOptions& options);

  // Set/reset the runner options.
  //
  // This is useful for running a different gstreamer pipeline.
  // Calls can only succeed if the runner hasn't started.
  //
  // The options set do not change across state boundaries unless you explicitly
  // change it again with another call to this function.
  Status SetOptions(const GstreamerRunnerOptions& options);

  // Set/reset the callback to receive the processed result.
  //
  // This is useful for changing the receiver when the pipeline changes.
  // Calls can only succeed if the runner hasn't started.
  //
  // The callback is initialized to a no-op.
  //
  // The callback that is set does not unless you explicitly make another call.
  Status SetReceiver(const ReceiverCallback&);

  // Start the runner.
  //
  // You should call End() when you are done to reclaim the resources.
  Status Start();

  // Returns true if the runner has started. Otherwise, false.
  bool IsStarted() const;

  // Feed a GstreamerBuffer object for processing.
  Status Feed(const GstreamerBuffer&);

  // End the runner.
  Status End();

  // Copy-control members.
  ~GstreamerRunner();
  GstreamerRunner(const GstreamerRunner&) = delete;
  GstreamerRunner& operator=(const GstreamerRunner&) = delete;
  GstreamerRunner(GstreamerRunner&&) = delete;
  GstreamerRunner& operator=(GstreamerRunner&&) = delete;

 private:
  GstreamerRunnerOptions options_;
  ReceiverCallback receiver_callback_;
  bool is_gst_init_ = false;

  class GstreamerRuntimeImpl;
  std::unique_ptr<GstreamerRuntimeImpl> gstreamer_runtime_impl_;
};

}  // namespace aistreams

#endif  // AISTREAMS_GSTREAMER_GSTREAMER_RUNNER_H_
