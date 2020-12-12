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

#include "aistreams/util/random_string.h"

#include "absl/random/random.h"

namespace aistreams {

namespace {
constexpr char kRandomChars[] =
    "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
}  // namespace

std::string RandomString(size_t length) {
  thread_local static absl::BitGen bitgen;
  std::string result;
  result.resize(length);
  for (size_t i = 0; i < length; ++i) {
    size_t rand_i = absl::Uniform(bitgen, 0u, sizeof(kRandomChars) - 2);
    result[i] = kRandomChars[rand_i];
  }
  return result;
}

}  // namespace aistreams
