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

#ifndef AISTREAMS_BASE_UTIL_EXPONENTIAL_BACKOFF_H_
#define AISTREAMS_BASE_UTIL_EXPONENTIAL_BACKOFF_H_

#include "absl/time/time.h"

namespace aistreams {
class ExponentialBackoff {
 public:
  // Creates an exponential backoff class with the following parameters:
  // - initial_wait_time: time waited the first time Wait() is called.
  // - max_wait_time: maximum time to wait when Wait() is called.
  // - wait_time_multiplier: at each subsequent iteration, the wait time is
  // multiplied by this value (must be >= 1).
  ExponentialBackoff(absl::Duration initial_wait_time,
                     absl::Duration max_wait_time, float wait_time_multiplier);

  // Waits for the current wait time, and increments the wait time value.
  void Wait();

 private:
  absl::Duration initial_wait_time_;
  absl::Duration current_wait_time_;
  absl::Duration max_wait_time_;
  float wait_time_multiplier_;
};
}  // namespace aistreams

#endif