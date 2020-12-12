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

#include "aistreams/util/completion_signal.h"

#include <thread>

#include "absl/time/time.h"
#include "aistreams/port/canonical_errors.h"
#include "aistreams/port/gtest.h"
#include "aistreams/port/logging.h"
#include "aistreams/port/status.h"

namespace aistreams {

TEST(CompletionSignal, BasicTest) {
  CompletionSignal signal;
  signal.Start();
  std::thread worker([&signal]() {
    LOG(INFO) << "sleeping...";
    absl::SleepFor(absl::Seconds(2));
    LOG(INFO) << "awake...";
    signal.SetStatus(UnknownError("Bogus error"));
    signal.End();
  });
  while (!signal.WaitUntilCompleted(absl::Seconds(1))) {
    LOG(INFO) << signal.IsCompleted();
    LOG(INFO) << "waiting...";
  }
  EXPECT_FALSE(signal.GetStatus().ok());
  worker.join();
}

}  // namespace aistreams
