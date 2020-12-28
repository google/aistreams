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

#include "aistreams/util/status/ret_check.h"

namespace aistreams {

::aistreams::StatusBuilder RetCheckFailSlowPath(
    ::aistreams::SourceLocation location) {
  // TODO Implement LogWithStackTrace().
  return ::aistreams::InternalErrorBuilder(location)
         << "RET_CHECK failure (" << location.file_name() << ":"
         << location.line() << ") ";
}

::aistreams::StatusBuilder RetCheckFailSlowPath(
    ::aistreams::SourceLocation location, const char* condition) {
  return ::aistreams::RetCheckFailSlowPath(location) << condition;
}

::aistreams::StatusBuilder RetCheckFailSlowPath(
    ::aistreams::SourceLocation location, const char* condition,
    const ::aistreams::Status& status) {
  return ::aistreams::RetCheckFailSlowPath(location)
         << condition << " returned " << status << " ";
}

}  // namespace aistreams
