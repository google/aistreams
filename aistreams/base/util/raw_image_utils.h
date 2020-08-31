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

#ifndef AISTREAMS_BASE_UTIL_RAW_IMAGE_UTILS_H_
#define AISTREAMS_BASE_UTIL_RAW_IMAGE_UTILS_H_

#include "absl/strings/string_view.h"
#include "aistreams/base/types/raw_image.h"
#include "aistreams/port/status.h"

namespace aistreams {

// Write the given RawImage as a PPM file.
Status ToPpmFile(absl::string_view file_name, const RawImage&);

// Read the given PPM file as a RawImage.
//
// The given image must have been written by ToPpmFile.
Status FromPpmFile(absl::string_view file_name, RawImage*);

}  // namespace aistreams

#endif  // AISTREAMS_BASE_UTIL_RAW_IMAGE_UTILS_H_
