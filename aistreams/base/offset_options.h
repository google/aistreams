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

#include "absl/time/time.h"
#include "absl/types/variant.h"

namespace aistreams {

// Options for a receiver to set its offset.
struct OffsetOptions {
  // A set of special offset values.
  enum class SpecialOffset {
    kOffsetBeginning,
    kOffsetEnd,
  };

  // If this holds an `int64_t` value, then the receiver will start receiving
  // packets from the corresponding offset position; it this holds an
  // `absl::Time`, then the receiver will start from the latest packet with
  // timestamp earlier than this value.
  using PositionType = absl::variant<SpecialOffset, int64_t, absl::Time>;

  // Whether the receiver wants to reset the options. If this is false, then
  // values of all the other fields will be ignored.
  bool reset_offset = false;

  // The position to start receiving packet from.
  PositionType offset_position;
};

}  // namespace aistreams

#endif  // AISTREAMS_BASE_OFFSET_OPTIONS_H_
