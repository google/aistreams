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

#include "aistreams/base/util/exponential_backoff.h"

#include "absl/time/clock.h"

namespace aistreams {

using ::absl::Duration;
using ::absl::ZeroDuration;

ExponentialBackoff::ExponentialBackoff(absl::Duration initial_wait_time,
                                       absl::Duration max_wait_time,
                                       float wait_time_multiplier) {
  // Initial wait time should not be negative.
  // TODO: set positive value for initial_wait_time_.
  initial_wait_time_ = std::max(initial_wait_time, ZeroDuration());
  current_wait_time_ = initial_wait_time_;
  // Maximum wait time should not be lower than the initial wait time.
  max_wait_time_ = std::max(max_wait_time, initial_wait_time_);
  // Wait time multiplier should not be less than 1.
  wait_time_multiplier_ = std::max(wait_time_multiplier, 1.0f);
}

void ExponentialBackoff::Wait() {
  absl::SleepFor(current_wait_time_);
  if (current_wait_time_ < max_wait_time_) {
    current_wait_time_ =
        std::min(wait_time_multiplier_ * current_wait_time_, max_wait_time_);
  }
}
}  // namespace aistreams