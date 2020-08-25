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

#ifndef AISTREAMS_BASE_TYPES_EOS_H_
#define AISTREAMS_BASE_TYPES_EOS_H_

#include <string>

#include "aistreams/port/status.h"
#include "aistreams/proto/types/control_signal.pb.h"

namespace aistreams {

// Eos is a control type that indicates "end-of-stream".
class Eos {
 public:
  // Constructors. A default constructed Eos has no preset reasons.
  Eos() = default;
  explicit Eos(const EosValue& value) : value_(value) {}

  // Returns the reason for this Eos.
  const std::string& reason() { return value_.reason(); }

  // Set the reason for this Eos.
  void set_reason(const std::string& reason) { value_.set_reason(reason); }

  // Get a reference to the full eos value.
  const EosValue& value() const { return value_; }

  // Set the eos value.
  void set_value(const EosValue& value) { value_ = value; }

 private:
  EosValue value_;
};

}  // namespace aistreams

#endif  // AISTREAMS_BASE_TYPES_EOS_H_
