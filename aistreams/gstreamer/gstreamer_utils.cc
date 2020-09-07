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

#include <gst/gst.h>

#include <string>

#include "absl/strings/str_format.h"
#include "aistreams/port/canonical_errors.h"
#include "aistreams/port/logging.h"
#include "aistreams/port/status.h"
#include "aistreams/port/statusor.h"

namespace aistreams {

namespace {
std::string print_gerror(GError *gerr) {
  if (gerr == nullptr) {
    return "";
  }
  return absl::StrFormat("%s (%d): %s", g_quark_to_string(gerr->domain),
                         gerr->code, gerr->message);
}
}  // namespace

Status GstInit() {
  GError *gerr = NULL;
  if (gst_init_check(nullptr, nullptr, &gerr) == FALSE) {
    auto s = InternalError(print_gerror(gerr));
    g_clear_error(&gerr);
    return s;
  }
  return OkStatus();
}

Status GstLaunchPipeline(const std::string &gst_pipeline) {
  // Initialize gstreamer if requested.
  auto status = GstInit();
  if (!status.ok()) {
    LOG(ERROR) << status;
    return InternalError("Could not initialize Gstreamer");
  }

  // Build the pipeline.
  GError *gerr = nullptr;
  GstElement *pipeline = gst_parse_launch(gst_pipeline.c_str(), &gerr);
  if (gerr != nullptr) {
    auto s = InternalError(print_gerror(gerr));
    g_clear_error(&gerr);
    return s;
  }

  // Play the pipeline.
  if (gst_element_set_state(pipeline, GST_STATE_PLAYING) ==
      GST_STATE_CHANGE_FAILURE) {
    return InternalError("Failed to start playing the pipeline");
  }

  // Observe the message bus for when GST_MESSAGE_ERROR or GST_MESSAGE_EOS
  // shows up so that we can stop the pipeline.
  GstBus *bus = gst_element_get_bus(pipeline);
  if (bus == nullptr) {
    return InternalError("Failed to get the message bus");
  }
  GstMessage *msg = gst_bus_timed_pop_filtered(
      bus, GST_CLOCK_TIME_NONE,
      static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

  // Stop the pipeline and clean up.
  if (msg != nullptr) {
    gst_message_unref(msg);
  }
  gst_object_unref(bus);
  gst_element_set_state(pipeline, GST_STATE_NULL);
  gst_object_unref(pipeline);
  return OkStatus();
}

}  // namespace aistreams
