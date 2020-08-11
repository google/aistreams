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

#ifndef AISTREAMS_UTIL_FILE_HELPERS_H_
#define AISTREAMS_UTIL_FILE_HELPERS_H_

#include "absl/strings/match.h"
#include "aistreams/port/status.h"

namespace aistreams {
namespace file {

// Reads the contents of the file `file_name` into `output`.
//
// REQUIRES: `output` is non-null.
Status GetContents(absl::string_view file_name, std::string* output);

// Writes the data provided in `content` to the file `file_name`. It overwrites
// any existing content.
Status SetContents(absl::string_view file_name, absl::string_view content);

// Decides if the named path `file_name` exists.
Status Exists(absl::string_view file_name);

}  // namespace file
}  // namespace aistreams

#endif  // AISTREAMS_UTIL_FILE_HELPERS_H_
