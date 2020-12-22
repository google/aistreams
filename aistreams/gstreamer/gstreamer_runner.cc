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

#include <gst/app/gstappsink.h>
#include <gst/gst.h>

#include <algorithm>
#include <thread>
#include <utility>

#include "absl/strings/str_format.h"
#include "absl/strings/str_join.h"
#include "absl/synchronization/mutex.h"
#include "aistreams/gstreamer/gstreamer_utils.h"
#include "aistreams/port/canonical_errors.h"
#include "aistreams/port/logging.h"
#include "aistreams/port/status.h"
#include "aistreams/port/status_macros.h"
#include "aistreams/util/completion_signal.h"

namespace aistreams {

namespace {

constexpr char kAppSrcName[] = "feed";
constexpr char kAppSinkName[] = "fetch";
constexpr int kPipelineFinishTimeoutSeconds = 5;

// RAII object that grants a *running* glib main loop.
//
// The event loop is run in a background thread. Gstreamer's Bus mechanism works
// as long as there is some glib main loop running; i.e. it needn't be in the
// main thread.
class GMainLoopManager {
 public:
  GMainLoopManager() {
    glib_main_loop_ = g_main_loop_new(NULL, FALSE);
    glib_main_loop_runner_ =
        std::thread([this]() { g_main_loop_run(glib_main_loop_); });
    while (g_main_loop_is_running(glib_main_loop_) != TRUE)
      ;
  }

  ~GMainLoopManager() {
    g_main_loop_quit(glib_main_loop_);
    glib_main_loop_runner_.join();
    g_main_loop_unref(glib_main_loop_);
  }

  GMainLoopManager(const GMainLoopManager&) = delete;
  GMainLoopManager(GMainLoopManager&&) = delete;
  GMainLoopManager& operator=(const GMainLoopManager&) = delete;

 private:
  GMainLoop* glib_main_loop_ = nullptr;
  std::thread glib_main_loop_runner_;
};

// Callback attached to observe pipeline bus messages.
gboolean gst_bus_message_callback(GstBus* bus, GstMessage* message,
                                  CompletionSignal* signal) {
  GError* err;
  gchar* debug_info;
  switch (GST_MESSAGE_TYPE(message)) {
    case GST_MESSAGE_EOS:
      signal->End();
      break;
    case GST_MESSAGE_ERROR:
      gst_message_parse_error(message, &err, &debug_info);
      LOG(ERROR) << absl::StrFormat("Error from gstreamer element %s: %s",
                                    GST_OBJECT_NAME(message->src),
                                    err->message);
      LOG(ERROR) << absl::StrFormat("Additional debug info: %s",
                                    debug_info ? debug_info : "none");
      LOG(ERROR) << "Got gstreamer error; shutting down event loop";
      signal->End();
      break;
    default:
      break;
  }
  return TRUE;
}

// Callback for receiving new GstSample's from appsink.
GstFlowReturn on_new_sample_from_sink(
    GstElement* elt, GstreamerRunner::ReceiverCallback* receiver_callback) {
  // Get the GstSample from appsink.
  GstSample* sample = gst_app_sink_pull_sample(GST_APP_SINK(elt));

  // No-op if callbacks are not supplied.
  if (receiver_callback == nullptr || !(*receiver_callback)) {
    gst_sample_unref(sample);
    return GST_FLOW_OK;
  }

  // Copy the GstSample into aistreamer's GstreamerBuffer type.
  GstreamerBuffer gstreamer_buffer;

  GstCaps* caps = gst_sample_get_caps(sample);
  gchar* caps_string = gst_caps_to_string(caps);
  gstreamer_buffer.set_caps_string(caps_string);
  g_free(caps_string);

  GstBuffer* buffer = gst_sample_get_buffer(sample);
  GstMapInfo map;
  gst_buffer_map(buffer, &map, GST_MAP_READ);
  gstreamer_buffer.assign(reinterpret_cast<char*>(map.data), map.size);
  gst_buffer_unmap(buffer, &map);
  gst_sample_unref(sample);

  // Deliver the GstreamerBuffer using the callback.
  // TODO: Decide on special status codes to pause/halt the pipeline.
  Status status = (*receiver_callback)(std::move(gstreamer_buffer));
  if (!status.ok()) {
    LOG(ERROR) << status.message();
    return GST_FLOW_ERROR;
  }
  return GST_FLOW_OK;
}

Status ValidateOptions(const GstreamerRunner::Options& options) {
  if (options.processing_pipeline_string.empty()) {
    return InvalidArgumentError("Given an empty processing pipeline string");
  }
  return OkStatus();
}

// Object that owns and configures a gstreamer pipeline.
//
// It supports pipelines of the form:
// gst-launch [appsrc !] main-processing-pipeline [! appsink]
//
// appsrc and appsink are added depending on whether appsrc caps and a callback
// is provided in Options.
class GstreamerPipeline {
 public:
  static StatusOr<std::unique_ptr<GstreamerPipeline>> Create(
      const GstreamerRunner::Options& options) {
    auto status = ValidateOptions(options);
    if (!status.ok()) {
      LOG(ERROR) << status;
      return InvalidArgumentError(
          "The given GstreamerRunner::Options has errors");
    }
    auto gstreamer_pipeline = std::make_unique<GstreamerPipeline>();

    // Create the gst_pipeline.
    std::vector<std::string> pipeline_elements;
    if (!options.appsrc_caps_string.empty()) {
      // TODO: might be useful to have an option to configure this string.
      pipeline_elements.push_back(absl::StrFormat(
          "appsrc name=%s is-live=true do-timestamp=true format=3",
          kAppSrcName));
    }

    if (options.processing_pipeline_string.empty()) {
      return InvalidArgumentError("Given an empty processing pipeline string");
    }
    pipeline_elements.push_back(options.processing_pipeline_string);

    if (options.receiver_callback) {
      pipeline_elements.push_back(
          absl::StrFormat("appsink name=%s", kAppSinkName));
    }
    std::string pipeline_string = absl::StrJoin(pipeline_elements, " ! ");

    gstreamer_pipeline->gst_pipeline_ =
        gst_parse_launch(pipeline_string.c_str(), NULL);
    if (gstreamer_pipeline->gst_pipeline_ == nullptr) {
      return InvalidArgumentError(
          absl::StrFormat("Failed to create a gstreamer pipeline using "
                          "\"%s\". Make sure you've "
                          "given a valid processing pipeline string",
                          pipeline_string));
    }

    // Configure the appsrc.
    if (!options.appsrc_caps_string.empty()) {
      gstreamer_pipeline->gst_appsrc_ = gst_bin_get_by_name(
          GST_BIN(gstreamer_pipeline->gst_pipeline_), kAppSrcName);
      if (gstreamer_pipeline->gst_appsrc_ == nullptr) {
        return InternalError("Failed to get a pointer to the appsrc element");
      }
      GstCaps* appsrc_caps =
          gst_caps_from_string(options.appsrc_caps_string.c_str());
      if (appsrc_caps == nullptr) {
        return InvalidArgumentError(absl::StrFormat(
            "Failed to create a GstCaps from \"%s\"; make sure it is a valid "
            "cap string",
            options.appsrc_caps_string));
      }
      g_object_set(G_OBJECT(gstreamer_pipeline->gst_appsrc_), "caps",
                   appsrc_caps, NULL);
      gst_caps_unref(appsrc_caps);
    }

    // Configure the appsink.
    if (options.receiver_callback) {
      gstreamer_pipeline->gst_appsink_ = gst_bin_get_by_name(
          GST_BIN(gstreamer_pipeline->gst_pipeline_), kAppSinkName);
      if (gstreamer_pipeline->gst_appsink_ == nullptr) {
        return InternalError("Failed to get a pointer to the appsink element");
      }
      g_object_set(G_OBJECT(gstreamer_pipeline->gst_appsink_), "emit-signals",
                   TRUE, "sync", options.appsink_sync ? TRUE : FALSE, NULL);
      g_signal_connect(gstreamer_pipeline->gst_appsink_, "new-sample",
                       G_CALLBACK(on_new_sample_from_sink),
                       const_cast<GstreamerRunner::ReceiverCallback*>(
                           &options.receiver_callback));
    }

    return gstreamer_pipeline;
  }

  GstElement* gst_pipeline() const { return gst_pipeline_; }

  GstElement* gst_appsrc() const { return gst_appsrc_; }

  GstreamerPipeline() = default;
  ~GstreamerPipeline() { Cleanup(); }
  GstreamerPipeline(const GstreamerPipeline&) = delete;
  GstreamerPipeline& operator=(const GstreamerPipeline&) = delete;

 private:
  void Cleanup() {
    if (gst_pipeline_ != nullptr) {
      gst_object_unref(gst_pipeline_);
    }
    if (gst_appsrc_ != nullptr) {
      gst_object_unref(gst_appsrc_);
    }
    if (gst_appsink_ != nullptr) {
      gst_object_unref(gst_appsink_);
    }
  }

  GstElement* gst_pipeline_ = nullptr;
  GstElement* gst_appsrc_ = nullptr;
  GstElement* gst_appsink_ = nullptr;
};

}  // namespace

// -----------------------------------------------------------------------
// GstreamerRunnerImpl

// A class that manages a running Gstreamer pipeline.
class GstreamerRunner::GstreamerRunnerImpl {
 public:
  // Create a fully initialized and running gstreamer pipeline.
  static StatusOr<std::unique_ptr<GstreamerRunnerImpl>> Create(
      const Options& options);

  // Feed a GstreamerBuffer into the running pipeline.
  Status Feed(const GstreamerBuffer&);

  bool IsCompleted() { return completion_signal_->IsCompleted(); }

  bool WaitUntilCompleted(absl::Duration timeout) const {
    return completion_signal_->WaitUntilCompleted(timeout);
  }

  GstreamerRunnerImpl(const Options& options) : options_(options) {}
  ~GstreamerRunnerImpl();

 private:
  Status Initialize();
  Status Finalize();

  Options options_;

  std::unique_ptr<GstreamerPipeline> gstreamer_pipeline_ = nullptr;
  std::unique_ptr<CompletionSignal> completion_signal_ = nullptr;
  std::unique_ptr<GMainLoopManager> glib_loop_manager_ = nullptr;
};

StatusOr<std::unique_ptr<GstreamerRunner::GstreamerRunnerImpl>>
GstreamerRunner::GstreamerRunnerImpl::Create(const Options& options) {
  auto runner_impl = std::make_unique<GstreamerRunnerImpl>(options);
  AIS_RETURN_IF_ERROR(runner_impl->Initialize());
  return runner_impl;
}

Status GstreamerRunner::GstreamerRunnerImpl::Initialize() {
  // Create the gstreamer pipeline.
  auto gstreamer_pipeline_statusor = GstreamerPipeline::Create(options_);
  if (!gstreamer_pipeline_statusor.ok()) {
    return gstreamer_pipeline_statusor.status();
  }
  gstreamer_pipeline_ = std::move(gstreamer_pipeline_statusor).ValueOrDie();

  // Create the completion signal to observe the pipeline progress.
  completion_signal_ = std::make_unique<CompletionSignal>();
  GstBus* bus = gst_element_get_bus(gstreamer_pipeline_->gst_pipeline());
  gst_bus_add_watch(bus, (GstBusFunc)gst_bus_message_callback,
                    completion_signal_.get());
  gst_object_unref(bus);

  // Start the pipeline.
  completion_signal_->Start();
  glib_loop_manager_ = std::make_unique<GMainLoopManager>();
  gst_element_set_state(gstreamer_pipeline_->gst_pipeline(), GST_STATE_PLAYING);
  return OkStatus();
}

Status GstreamerRunner::GstreamerRunnerImpl::Finalize() {
  // Allow the pipeline to complete its processing gracefully.
  // Do this by sending it EOS and enforcing a deadline.
  if (!completion_signal_->IsCompleted()) {
    if (gstreamer_pipeline_->gst_appsrc()) {
      GstFlowReturn ret;
      g_signal_emit_by_name(gstreamer_pipeline_->gst_appsrc(), "end-of-stream",
                            &ret);
    } else {
      gst_element_send_event(gstreamer_pipeline_->gst_pipeline(),
                             gst_event_new_eos());
    }
    if (!completion_signal_->WaitUntilCompleted(
            absl::Seconds(kPipelineFinishTimeoutSeconds))) {
      LOG(WARNING) << "The gstreamer pipeline could not complete its cleanup "
                      "executions within the timeout ( "
                   << kPipelineFinishTimeoutSeconds
                   << "s). Discarding to move on; consumers might experince "
                      "dropped results";
    }
  }
  gst_element_set_state(gstreamer_pipeline_->gst_pipeline(), GST_STATE_NULL);

  return OkStatus();
}

GstreamerRunner::GstreamerRunnerImpl::~GstreamerRunnerImpl() {
  auto status = Finalize();
  if (!status.ok()) {
    LOG(ERROR) << status;
  }
}

Status GstreamerRunner::GstreamerRunnerImpl::Feed(
    const GstreamerBuffer& gstreamer_buffer) {
  if (IsCompleted()) {
    return FailedPreconditionError(
        "The runner has already completed. Please Create() it again and retry");
  }
  // Check that the pipeline is configured for Feeding.
  if (gstreamer_pipeline_->gst_appsrc() == nullptr) {
    return InvalidArgumentError("This runner is not configured for Feeding");
  }

  // Check that the given caps agree with those of the pipeline.
  if (gstreamer_buffer.get_caps() != options_.appsrc_caps_string) {
    return InvalidArgumentError(absl::StrFormat(
        "Feeding the runner with caps \"%s\" when \"%s\" is expected",
        gstreamer_buffer.get_caps(), options_.appsrc_caps_string));
  }

  // Create a new GstBuffer by copying.
  GstBuffer* buffer = gst_buffer_new_and_alloc(gstreamer_buffer.size());
  GstMapInfo map;
  gst_buffer_map(buffer, &map, GST_MAP_READWRITE);
  std::copy(gstreamer_buffer.data(),
            gstreamer_buffer.data() + gstreamer_buffer.size(), (char*)map.data);
  gst_buffer_unmap(buffer, &map);

  // Feed the buffer.
  GstFlowReturn ret;
  g_signal_emit_by_name(gstreamer_pipeline_->gst_appsrc(), "push-buffer",
                        buffer, &ret);
  gst_buffer_unref(buffer);
  if (ret != GST_FLOW_OK) {
    return InternalError("Failed to push a GstBuffer");
  }
  return OkStatus();
}

// -----------------------------------------------------------------------
// GstreamerRunner

GstreamerRunner::GstreamerRunner() = default;

GstreamerRunner::~GstreamerRunner() = default;

StatusOr<std::unique_ptr<GstreamerRunner>> GstreamerRunner::Create(
    const Options& options) {
  auto gstreamer_runner = std::make_unique<GstreamerRunner>();

  // Initializes gstreamer if it isn't already.
  Status status = GstInit();
  if (!status.ok()) {
    LOG(ERROR) << status;
    return InternalError("Could not initialize GStreamer");
  }

  // Create a GstreamerRunnerImpl.
  auto gstreamer_runner_impl_statusor = GstreamerRunnerImpl::Create(options);
  if (!gstreamer_runner_impl_statusor.ok()) {
    LOG(ERROR) << gstreamer_runner_impl_statusor.status();
    return UnknownError("Failed to create a gstreamer runner");
  }
  gstreamer_runner->gstreamer_runner_impl_ =
      std::move(gstreamer_runner_impl_statusor).ValueOrDie();
  return gstreamer_runner;
}

Status GstreamerRunner::Feed(const GstreamerBuffer& gstreamer_buffer) const {
  Status status = gstreamer_runner_impl_->Feed(gstreamer_buffer);
  if (!status.ok()) {
    LOG(ERROR) << status;
    return UnknownError("Failed to Feed the GstreamerRunner");
  }
  return OkStatus();
}

bool GstreamerRunner::IsCompleted() const {
  return gstreamer_runner_impl_->IsCompleted();
}

bool GstreamerRunner::WaitUntilCompleted(absl::Duration timeout) const {
  return gstreamer_runner_impl_->WaitUntilCompleted(timeout);
}

}  // namespace aistreams
