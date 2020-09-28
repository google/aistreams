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
// `play_duration_in_seconds`: This is the maximum amount of time to run the
//                             pipeline for. Set it to -1 if you do not want a
//                             cap.
//                             The run will always end if the pipeline itself
//                             signals EOS, even if the bound hasn't been
//                             reached. (e.g. if the input source actually
//                             ends).
//
// The single argument overload applies no bound to the play duration (it just
// passes -1 to play_duration_in_seconds).
Status GstLaunchPipeline(const std::string& gst_pipeline);
Status GstLaunchPipeline(const std::string& gst_pipeline,
                         int play_duration_in_seconds);

}  // namespace aistreams

#endif  // AISTREAMS_GSTREAMER_GSTREAMER_UTILS_H_
