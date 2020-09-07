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

#ifndef AISTREAMS_GSTREAMER_GSTREAMER_UTILS_H_
#define AISTREAMS_GSTREAMER_GSTREAMER_UTILS_H_

#include <string>

#include "aistreams/port/status.h"
#include "aistreams/port/statusor.h"

namespace aistreams {

// Initialize GStreamer if it hasn't already been.
//
// This calls gst_init_check and returns the result through Status.
// You should call this rather than gst_init() directly.
Status GstInit();

// Launch a gstreamer pipeline and block until it is done.
//
// `gst_pipeline`: This is a string that you would normally pass to gst-launch.
Status GstLaunchPipeline(const std::string& gst_pipeline);

}  // namespace aistreams

#endif  // AISTREAMS_GSTREAMER_GSTREAMER_UTILS_H_
