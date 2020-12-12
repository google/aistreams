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

#include "aistreams/util/completion_signal.h"

namespace aistreams {

CompletionSignal::CompletionSignal() = default;
CompletionSignal::~CompletionSignal() = default;

void CompletionSignal::Start() {
  absl::MutexLock lock(&mu_);
  is_completed_ = false;
}

void CompletionSignal::End() {
  absl::MutexLock lock(&mu_);
  is_completed_ = true;
}

bool CompletionSignal::IsCompleted() const {
  absl::MutexLock lock(&mu_);
  return is_completed_;
}

bool CompletionSignal::WaitUntilCompleted(absl::Duration timeout) {
  absl::MutexLock lock(&mu_);
  absl::Condition cond(
      +[](bool* is_completed) -> bool { return *is_completed; },
      &is_completed_);
  return mu_.AwaitWithTimeout(cond, timeout);
}

Status CompletionSignal::GetStatus() const {
  absl::MutexLock lock(&mu_);
  return status_;
}

void CompletionSignal::SetStatus(const Status& status) {
  absl::MutexLock lock(&mu_);
  status_ = status;
  return;
}

}  // namespace aistreams
