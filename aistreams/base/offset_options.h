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

#ifndef AISTREAMS_BASE_OFFSET_OPTIONS_H_
#define AISTREAMS_BASE_OFFSET_OPTIONS_H_

namespace aistreams {

// Options for a receiver to set its offset.
struct OffsetOptions {
  // Whether the receiver wants to reset the options. If this is false, then
  // values of all the other fields will be ignored.
  bool reset_offset = false;

  // Whether the receiver want to start receiving from the latest packet. If
  // this is true, then the value of position will be ignored.
  bool from_latest = false;

  // The position of the packet that a receiver wants to start receiving from.
  int position;
};

}  // namespace aistreams

#endif  // AISTREAMS_BASE_OFFSET_OPTIONS_H_
