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
#include "absl/synchronization/mutex.h"
#include "aistreams/gstreamer/gstreamer_utils.h"
#include "aistreams/port/canonical_errors.h"
#include "aistreams/port/logging.h"
#include "aistreams/port/status.h"
#include "aistreams/port/status_macros.h"

namespace aistreams {

namespace {

constexpr char kAppSrcName[] = "feed";
constexpr char kAppSinkName[] = "fetch";
constexpr int kPipelineFinishTimeoutSeconds = 5;

// An object used to signal completion to collaborating threads.
class CompletionSignal {
 public:
  CompletionSignal() = default;
  ~CompletionSignal() = default;

  void Start() {
    absl::MutexLock lock(&is_completed_mu_);
    is_completed_ = false;
  }

  void End() {
    absl::MutexLock lock(&is_completed_mu_);
    is_completed_ = true;
  }

  bool IsCompleted() {
    absl::MutexLock lock(&is_completed_mu_);
    return is_completed_;
  }

  bool WaitUntilComplete(absl::Duration timeout) {
    absl::MutexLock lock(&is_completed_mu_);
    absl::Condition cond(
        +[](bool* is_completed) -> bool { return *is_completed; },
        &is_completed_);
    return is_completed_mu_.AwaitWithTimeout(cond, timeout);
  }

  CompletionSignal(const CompletionSignal&) = delete;
  CompletionSignal(CompletionSignal&&) = delete;
  CompletionSignal& operator=(const CompletionSignal&) = delete;

 private:
  mutable absl::Mutex is_completed_mu_;
  bool is_completed_ ABSL_GUARDED_BY(is_completed_mu_) = true;
};

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
  }
  return GST_FLOW_OK;
}

Status ValidateRunnerOptions(const GstreamerRunnerOptions& options) {
  if (options.processing_pipeline_string.empty()) {
    return InvalidArgumentError("Given an empty processing pipeline string");
  }
  if (options.appsrc_caps_string.empty()) {
    return InvalidArgumentError("Given an empty appsrc caps string");
  }
  return OkStatus();
}

}  // namespace

// A class that manages a running Gstreamer pipeline.
class GstreamerRunner::GstreamerRuntimeImpl {
 public:
  struct Options {
    std::string processing_pipeline_string;
    std::string appsrc_caps_string;
    ReceiverCallback receiver_callback;
  };

  // Create a fully initialized and running gstreamer pipeline.
  static StatusOr<std::unique_ptr<GstreamerRuntimeImpl>> Create(
      const Options& options) {
    auto runtime_impl = std::make_unique<GstreamerRuntimeImpl>(options);
    AIS_RETURN_IF_ERROR(runtime_impl->Initialize());
    return runtime_impl;
  }

  // Feed a GstreamerBuffer into the running pipeline.
  Status Feed(const GstreamerBuffer&);

  GstreamerRuntimeImpl(const Options& options) : options_(options) {}
  ~GstreamerRuntimeImpl();

 private:
  Status Initialize();
  Status Finalize();

  Options options_;

  GstElement* gst_pipeline_ = nullptr;
  GstElement* gst_appsrc_ = nullptr;
  GstElement* gst_appsink_ = nullptr;

  std::unique_ptr<CompletionSignal> completion_signal_ = nullptr;
  std::unique_ptr<GMainLoopManager> glib_loop_manager_ = nullptr;
};

Status GstreamerRunner::GstreamerRuntimeImpl::Initialize() {
  // Create the full gstreamer pipeline.
  std::string pipeline_string = absl::StrFormat(
      "appsrc name=%s is-live=true ! %s ! appsink name=%s", kAppSrcName,
      options_.processing_pipeline_string, kAppSinkName);
  gst_pipeline_ = gst_parse_launch(pipeline_string.c_str(), NULL);
  if (gst_pipeline_ == nullptr) {
    return InvalidArgumentError(absl::StrFormat(
        "Failed to create a gstreamer pipeline using \"%s\". Make sure you've "
        "given a valid processing pipeline string",
        pipeline_string));
  }

  // Setup the appsrc.
  gst_appsrc_ = gst_bin_get_by_name(GST_BIN(gst_pipeline_), kAppSrcName);
  if (gst_appsrc_ == nullptr) {
    return InternalError("Failed to get a pointer to the appsrc element");
  }
  GstCaps* appsrc_caps =
      gst_caps_from_string(options_.appsrc_caps_string.c_str());
  if (appsrc_caps == nullptr) {
    return InvalidArgumentError(absl::StrFormat(
        "Failed to create a GstCaps from \"%s\"; make sure it is a valid "
        "cap string",
        options_.appsrc_caps_string));
  }
  g_object_set(G_OBJECT(gst_appsrc_), "caps", appsrc_caps, NULL);
  gst_caps_unref(appsrc_caps);

  // Setup the appsink.
  gst_appsink_ = gst_bin_get_by_name(GST_BIN(gst_pipeline_), kAppSinkName);
  if (gst_appsink_ == nullptr) {
    return InternalError("Failed to get a pointer to the appsink element");
  }
  g_object_set(G_OBJECT(gst_appsink_), "emit-signals", TRUE, "sync", FALSE,
               NULL);
  g_signal_connect(gst_appsink_, "new-sample",
                   G_CALLBACK(on_new_sample_from_sink),
                   &options_.receiver_callback);

  // Observe the pipeline bus.
  completion_signal_ = std::make_unique<CompletionSignal>();
  GstBus* bus = gst_element_get_bus(gst_pipeline_);
  gst_bus_add_watch(bus, (GstBusFunc)gst_bus_message_callback,
                    completion_signal_.get());
  gst_object_unref(bus);

  // Start the pipeline.
  completion_signal_->Start();
  glib_loop_manager_ = std::make_unique<GMainLoopManager>();
  gst_element_set_state(gst_pipeline_, GST_STATE_PLAYING);

  return OkStatus();
}

Status GstreamerRunner::GstreamerRuntimeImpl::Finalize() {
  // Allow the pipeline to complete its processing gracefully.
  // Do this by sending it EOS and enforcing a deadline.
  if (!completion_signal_->IsCompleted()) {
    GstFlowReturn ret;
    g_signal_emit_by_name(gst_appsrc_, "end-of-stream", &ret);
    if (!completion_signal_->WaitUntilComplete(
            absl::Seconds(kPipelineFinishTimeoutSeconds))) {
      LOG(WARNING) << "The gstreamer pipeline could not complete its cleanup "
                      "executions within the timeout ( "
                   << kPipelineFinishTimeoutSeconds
                   << "s). Discarding to move on; consumers might experince "
                      "dropped results";
    }
  }
  gst_element_set_state(gst_pipeline_, GST_STATE_NULL);

  // Cleanup.
  gst_object_unref(gst_appsink_);
  gst_object_unref(gst_appsrc_);
  gst_object_unref(gst_pipeline_);

  return OkStatus();
}

GstreamerRunner::GstreamerRuntimeImpl::~GstreamerRuntimeImpl() {
  auto status = Finalize();
  if (!status.ok()) {
    LOG(ERROR) << status;
  }
}

Status GstreamerRunner::GstreamerRuntimeImpl::Feed(
    const GstreamerBuffer& gstreamer_buffer) {
  // Check that the given caps agree with those of the pipeline.
  //
  // Our way of supporting caps changes is to bringup/teardown the
  // GstreamerRuntimeImpl itself rather than deal with the nuances of doing this
  // in situ from within Gstreamer.
  if (gstreamer_buffer.get_caps() != options_.appsrc_caps_string) {
    return InvalidArgumentError(absl::StrFormat(
        "Feeding an appsrc with caps \"%s\" with a data of caps \"%s\"",
        options_.appsrc_caps_string, gstreamer_buffer.get_caps()));
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
  g_signal_emit_by_name(gst_appsrc_, "push-buffer", buffer, &ret);
  gst_buffer_unref(buffer);
  if (ret != GST_FLOW_OK) {
    return InternalError("Failed to push a GstBuffer");
  }
  return OkStatus();
}

// -----------------------------------------------------------------------
// Begin GstreamerRunner implementation

GstreamerRunner::GstreamerRunner() = default;

GstreamerRunner::GstreamerRunner(const GstreamerRunnerOptions& options)
    : options_(options) {}

GstreamerRunner::~GstreamerRunner() {}

Status GstreamerRunner::SetOptions(const GstreamerRunnerOptions& options) {
  if (IsStarted()) {
    return FailedPreconditionError(
        "You cannot SetOptions while the runner has Started");
  }
  options_ = options;
  return OkStatus();
}

Status GstreamerRunner::SetReceiver(const ReceiverCallback& callback) {
  if (IsStarted()) {
    return FailedPreconditionError(
        "You cannot SetReceiver while the runner has Started");
  }
  receiver_callback_ = callback;
  return OkStatus();
}

bool GstreamerRunner::IsStarted() const {
  return gstreamer_runtime_impl_ != nullptr;
}

Status GstreamerRunner::Start() {
  if (IsStarted()) {
    return FailedPreconditionError("The runner has already Started");
  }

  // Initialize gstreamer if not already.
  Status status = GstInit();
  if (!status.ok()) {
    LOG(ERROR) << status;
    return InternalError("Could not initialize GStreamer");
  }

  // Create a GstreamerRuntimeImpl.
  status = ValidateRunnerOptions(options_);
  if (!status.ok()) {
    LOG(ERROR) << status;
    return InvalidArgumentError("The given GstreamerRunnerOptions has errors");
  }
  GstreamerRuntimeImpl::Options options;
  options.processing_pipeline_string = options_.processing_pipeline_string;
  options.appsrc_caps_string = options_.appsrc_caps_string;
  options.receiver_callback = receiver_callback_;
  auto gstreamer_runtime_impl_statusor = GstreamerRuntimeImpl::Create(options);
  if (!gstreamer_runtime_impl_statusor.ok()) {
    LOG(ERROR) << gstreamer_runtime_impl_statusor.status();
    return UnknownError("Failed to create a gstreamer runtime");
  }
  gstreamer_runtime_impl_ =
      std::move(gstreamer_runtime_impl_statusor).ValueOrDie();

  return OkStatus();
}

Status GstreamerRunner::End() {
  if (!IsStarted()) {
    return FailedPreconditionError("The runner has already Ended");
  }
  gstreamer_runtime_impl_.reset(nullptr);
  return OkStatus();
}

Status GstreamerRunner::Feed(const GstreamerBuffer& gstreamer_buffer) {
  if (!IsStarted()) {
    return FailedPreconditionError("The runner has not been Started");
  }
  Status status = gstreamer_runtime_impl_->Feed(gstreamer_buffer);
  if (!status.ok()) {
    LOG(ERROR) << status;
    return UnknownError("Failed to Feed the GstreamerRunner");
  }
  return OkStatus();
}

}  // namespace aistreams
