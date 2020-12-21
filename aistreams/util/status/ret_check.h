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

#ifndef AISTREAMS_UTIL_STATUS_RET_CHECK_H_
#define AISTREAMS_UTIL_STATUS_RET_CHECK_H_

#include "absl/base/optimization.h"
#include "aistreams/util/status/status_builder.h"
#include "aistreams/util/status/status_macros.h"

namespace aistreams {
// Returns a StatusBuilder that corresponds to a `RET_CHECK` failure.
::aistreams::StatusBuilder RetCheckFailSlowPath(
    ::aistreams::SourceLocation location);

// Returns a StatusBuilder that corresponds to a `RET_CHECK` failure.
::aistreams::StatusBuilder RetCheckFailSlowPath(
    ::aistreams::SourceLocation location, const char* condition);

// Returns a StatusBuilder that corresponds to a `RET_CHECK` failure.
::aistreams::StatusBuilder RetCheckFailSlowPath(
    ::aistreams::SourceLocation location, const char* condition,
    const ::aistreams::Status& status);

inline StatusBuilder RetCheckImpl(const ::aistreams::Status& status,
                                  const char* condition,
                                  ::aistreams::SourceLocation location) {
  if (ABSL_PREDICT_TRUE(status.ok()))
    return ::aistreams::StatusBuilder(OkStatus(), location);
  return RetCheckFailSlowPath(location, condition, status);
}

}  // namespace aistreams

#define RET_CHECK(cond)               \
  while (ABSL_PREDICT_FALSE(!(cond))) \
  return ::aistreams::RetCheckFailSlowPath(AIS_LOC, #cond)

#define RET_CHECK_OK(status) \
  AIS_RETURN_IF_ERROR(::aistreams::RetCheckImpl((status), #status, AIS_LOC))

#define RET_CHECK_FAIL() return ::aistreams::RetCheckFailSlowPath(AIS_LOC)

#define AISTREAMS_INTERNAL_RET_CHECK_OP(name, op, lhs, rhs) \
  RET_CHECK((lhs)op(rhs))

#define RET_CHECK_EQ(lhs, rhs) AISTREAMS_INTERNAL_RET_CHECK_OP(EQ, ==, lhs, rhs)
#define RET_CHECK_NE(lhs, rhs) AISTREAMS_INTERNAL_RET_CHECK_OP(NE, !=, lhs, rhs)
#define RET_CHECK_LE(lhs, rhs) AISTREAMS_INTERNAL_RET_CHECK_OP(LE, <=, lhs, rhs)
#define RET_CHECK_LT(lhs, rhs) AISTREAMS_INTERNAL_RET_CHECK_OP(LT, <, lhs, rhs)
#define RET_CHECK_GE(lhs, rhs) AISTREAMS_INTERNAL_RET_CHECK_OP(GE, >=, lhs, rhs)
#define RET_CHECK_GT(lhs, rhs) AISTREAMS_INTERNAL_RET_CHECK_OP(GT, >, lhs, rhs)

#endif  // AISTREAMS_UTIL_STATUS_RET_CHECK_H_
