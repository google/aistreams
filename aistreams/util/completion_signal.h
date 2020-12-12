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

#ifndef AISTREAMS_UTIL_COMPLETION_SIGNAL_H_
#define AISTREAMS_UTIL_COMPLETION_SIGNAL_H_

#include "absl/synchronization/mutex.h"
#include "absl/time/time.h"
#include "aistreams/port/status.h"

namespace aistreams {

// CompletionSignal is a simple object used to communicate work progress and
// return status of an asynchronous worker to others.
//
// It is thread-safe.
//
// Example Usage
// -------------
//
// ```
// CompletionSignal signal;
//
// // Mark that the work is in progress and start the worker.
// signal.Start();
// std::thread worker([&signal]() {
//
//   // Do your thing.
//   Status work_status = DoImportantThing();
//
//   // When done, communicate the status and mark it complete.
//   signal.SetStatus(work_status);
//   signal.End();
//   return;
// });
//
// // Wait for the work to complete.
// while (!signal.WaitUntilCompleted(absl::Second(5)));
//
// // Check the worker's return status
// Status worker_status = signal.GetStatus();
// ```
class CompletionSignal {
 public:
  // Constructor.
  CompletionSignal();

  // Mark that the work is in progress.
  void Start();

  // Mark that the work has completed.
  void End();

  // Returns true if and only if work is not in progress.
  bool IsCompleted() const;

  // Blocks the caller until either the work has completed or if the specified
  // timeout has expired.
  //
  // Returns true if work is completed; otherwise, false.
  bool WaitUntilCompleted(absl::Duration timeout);

  // Get the status associated with this Signal.
  Status GetStatus() const;

  // Set the status associated with this Signal.
  void SetStatus(const Status& status);

  // Copy control.
  ~CompletionSignal();
  CompletionSignal(const CompletionSignal&) = delete;
  CompletionSignal& operator=(const CompletionSignal&) = delete;

 private:
  mutable absl::Mutex mu_;
  bool is_completed_ ABSL_GUARDED_BY(mu_) = true;
  Status status_ ABSL_GUARDED_BY(mu_) = OkStatus();
};

}  // namespace aistreams

#endif  // AISTREAMS_UTIL_COMPLETION_SIGNAL_H_
