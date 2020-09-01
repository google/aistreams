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
#include "aistreams/port/canonical_errors.h"
#include "aistreams/port/logging.h"
#include "aistreams/port/status.h"
#include "aistreams/port/status_macros.h"

namespace aistreams {

namespace {

constexpr char kAppSrcName[] = "feed";
constexpr char kAppSinkName[] = "fetch";

// Callback attached to the GLib main loop.
gboolean gst_bus_message_callback(GstBus* bus, GstMessage* message,
                                  GMainLoop* loop) {
  GError* err;
  gchar* debug_info;
  switch (GST_MESSAGE_TYPE(message)) {
    case GST_MESSAGE_EOS:
      g_main_loop_quit(loop);
      break;
    case GST_MESSAGE_ERROR:
      gst_message_parse_error(message, &err, &debug_info);
      LOG(ERROR) << absl::StrFormat("Error from gstreamer element %s: %s",
                                    GST_OBJECT_NAME(message->src),
                                    err->message);
      LOG(ERROR) << absl::StrFormat("Additional debug info: %s",
                                    debug_info ? debug_info : "none");
      LOG(ERROR) << "Got gstreamer error; shutting down event loop";
      g_main_loop_quit(loop);
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

// A class that manages the runtime/context of a Gstreamer pipeline.
class GstreamerRunner::GstreamerRuntimeImpl {
 public:
  struct Options {
    std::string processing_pipeline_string;
    std::string appsrc_caps_string;
    ReceiverCallback receiver_callback;
  };

  GstreamerRuntimeImpl(const Options& options) : options_(options) {}

  // Allocates and initializes a Gstreamer pipeline and any necessary resources
  // to support its execution.
  Status Initialize();

  // Feeds a GstreamerBuffer down the Gstreamer pipeline.
  Status Feed(const GstreamerBuffer&);

  // Frees the Gstreamer pipeline and any resources that was needed for its
  // executution.
  Status Finalize();

  ~GstreamerRuntimeImpl();

 private:
  Options options_;
  GstElement* gst_pipeline_ = nullptr;
  GMainLoop* glib_main_loop_ = nullptr;
  GstElement* gst_appsrc_ = nullptr;
  GstElement* gst_appsink_ = nullptr;
  std::thread glib_main_loop_runner_;
};

Status GstreamerRunner::GstreamerRuntimeImpl::Initialize() {
  // Create the full gstreamer pipeline.
  std::string pipeline_string =
      absl::StrFormat("appsrc name=%s ! %s ! appsink name=%s", kAppSrcName,
                      options_.processing_pipeline_string, kAppSinkName);
  gst_pipeline_ = gst_parse_launch(pipeline_string.c_str(), NULL);
  if (gst_pipeline_ == nullptr) {
    return InvalidArgumentError(absl::StrFormat(
        "Failed to create a gstreamer pipeline using \"%s\". Make sure you've "
        "given a valid processing pipeline string",
        pipeline_string));
  }

  // Create the GLib event loop.
  glib_main_loop_ = g_main_loop_new(NULL, FALSE);
  GstBus* bus = gst_element_get_bus(gst_pipeline_);
  gst_bus_add_watch(bus, (GstBusFunc)gst_bus_message_callback, glib_main_loop_);
  gst_object_unref(bus);

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

  // Play the gstreamer pipeline and start the glib main loop.
  gst_element_set_state(gst_pipeline_, GST_STATE_PLAYING);
  glib_main_loop_runner_ =
      std::thread([this]() { g_main_loop_run(glib_main_loop_); });

  return OkStatus();
}

Status GstreamerRunner::GstreamerRuntimeImpl::Finalize() {
  // Signal EOS. This will cause the pipeline to cease and trigger the GLib
  // event loop to stop.
  //
  // TODO(dschao): Check whether this works when the pipeline is already in an
  // error state. Find the "end-of-stream" handler in the gstreamer source.
  GstFlowReturn ret;
  g_signal_emit_by_name(gst_appsrc_, "end-of-stream", &ret);
  if (glib_main_loop_runner_.joinable()) {
    glib_main_loop_runner_.join();
  }

  // Set the pipeline to the null state.
  gst_element_set_state(gst_pipeline_, GST_STATE_NULL);

  // Cleanup.
  gst_object_unref(gst_appsink_);
  gst_object_unref(gst_appsrc_);
  g_main_loop_unref(glib_main_loop_);
  gst_object_unref(gst_pipeline_);

  return OkStatus();
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

GstreamerRunner::GstreamerRuntimeImpl::~GstreamerRuntimeImpl() {
  // Prevent the program from terminating if Finalize wasn't called/complete.
  if (glib_main_loop_runner_.joinable()) {
    glib_main_loop_runner_.detach();
  }
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

  // Initialize gstreamer itself only once.
  if (!is_gst_init_) {
    gst_init(nullptr, nullptr);
    is_gst_init_ = true;
  }

  // Create a GstreamerRuntimeImpl.
  Status status = ValidateRunnerOptions(options_);
  if (!status.ok()) {
    LOG(ERROR) << status;
    return InvalidArgumentError("The given GstreamerRunnerOptions has errors");
  }
  GstreamerRuntimeImpl::Options options;
  options.processing_pipeline_string = options_.processing_pipeline_string;
  options.appsrc_caps_string = options_.appsrc_caps_string;
  options.receiver_callback = receiver_callback_;
  gstreamer_runtime_impl_ = std::make_unique<GstreamerRuntimeImpl>(options);

  // Initialize the GstreamerRuntimeImpl.
  status = gstreamer_runtime_impl_->Initialize();
  if (!status.ok()) {
    LOG(ERROR) << status;

    // TODO(dschao): This could leak resources. Probably ok for most cases, but
    // would be nice if this could be friendlier.
    gstreamer_runtime_impl_.reset(nullptr);
    return UnknownError("Failed to Initialize a GstreamerRuntimeImpl");
  }

  return OkStatus();
}

Status GstreamerRunner::End() {
  if (!IsStarted()) {
    return FailedPreconditionError("The runner has already Ended");
  }
  Status status = gstreamer_runtime_impl_->Finalize();
  gstreamer_runtime_impl_.reset(nullptr);
  if (!status.ok()) {
    return UnknownError("Failed to Finalize a GstreamerRuntimeImpl");
  }
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
