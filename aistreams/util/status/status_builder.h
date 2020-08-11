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

#ifndef AISTREAMS_UTIL_STATUS_STATUS_BUILDER_H_
#define AISTREAMS_UTIL_STATUS_STATUS_BUILDER_H_

#include <sstream>

#include "absl/base/attributes.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "aistreams/util/status/source_location.h"
#include "aistreams/util/status/status.h"

namespace aistreams {

class ABSL_MUST_USE_RESULT StatusBuilder {
 public:
  StatusBuilder(const StatusBuilder& sb);
  StatusBuilder& operator=(const StatusBuilder& sb);
  // Creates a `StatusBuilder` based on an original status.  If logging is
  // enabled, it will use `location` as the location from which the log message
  // occurs.  A typical user will call this with `AISTREAMS_LOC`.
  StatusBuilder(const ::aistreams::Status& original_status,
                ::aistreams::source_location location)
      : status_(original_status),
        line_(location.line()),
        file_(location.file_name()),
        stream_(new std::ostringstream) {}

  StatusBuilder(::aistreams::Status&& original_status,
                ::aistreams::source_location location)
      : status_(std::move(original_status)),
        line_(location.line()),
        file_(location.file_name()),
        stream_(new std::ostringstream) {}

  // Creates a `StatusBuilder` from a aistreams status code.  If logging is
  // enabled, it will use `location` as the location from which the log message
  // occurs.  A typical user will call this with `AISTREAMS_LOC`.
  StatusBuilder(::aistreams::StatusCode code,
                ::aistreams::source_location location)
      : status_(code, ""),
        line_(location.line()),
        file_(location.file_name()),
        stream_(new std::ostringstream) {}

  StatusBuilder(const ::aistreams::Status& original_status, const char* file,
                int line)
      : status_(original_status),
        line_(line),
        file_(file),
        stream_(new std::ostringstream) {}

  bool ok() const { return status_.ok(); }

  StatusBuilder& SetAppend();

  StatusBuilder& SetPrepend();

  StatusBuilder& SetNoLogging();

  template <typename T>
  StatusBuilder& operator<<(const T& msg) {
    if (status_.ok()) return *this;
    *stream_ << msg;
    return *this;
  }

  operator Status() const&;
  operator Status() &&;

  ::aistreams::Status JoinMessageToStatus();

 private:
  // Specifies how to join the error message in the original status and any
  // additional message that has been streamed into the builder.
  enum class MessageJoinStyle {
    kAnnotate,
    kAppend,
    kPrepend,
  };

  // The status that the result will be based on.
  ::aistreams::Status status_;
  // The line to record if this file is logged.
  int line_;
  // Not-owned: The file to record if this status is logged.
  const char* file_;
  bool no_logging_ = false;
  // The additional messages added with `<<`.
  std::unique_ptr<std::ostringstream> stream_;
  // Specifies how to join the message in `status_` and `stream_`.
  MessageJoinStyle join_style_ = MessageJoinStyle::kAnnotate;
};

inline StatusBuilder AlreadyExistsErrorBuilder(
    ::aistreams::source_location location) {
  return StatusBuilder(::aistreams::StatusCode::kAlreadyExists, location);
}

inline StatusBuilder FailedPreconditionErrorBuilder(
    ::aistreams::source_location location) {
  return StatusBuilder(::aistreams::StatusCode::kFailedPrecondition, location);
}

inline StatusBuilder InternalErrorBuilder(
    ::aistreams::source_location location) {
  return StatusBuilder(::aistreams::StatusCode::kInternal, location);
}

inline StatusBuilder InvalidArgumentErrorBuilder(
    ::aistreams::source_location location) {
  return StatusBuilder(::aistreams::StatusCode::kInvalidArgument, location);
}

inline StatusBuilder NotFoundErrorBuilder(
    ::aistreams::source_location location) {
  return StatusBuilder(::aistreams::StatusCode::kNotFound, location);
}

inline StatusBuilder UnavailableErrorBuilder(
    ::aistreams::source_location location) {
  return StatusBuilder(::aistreams::StatusCode::kUnavailable, location);
}

inline StatusBuilder UnimplementedErrorBuilder(
    ::aistreams::source_location location) {
  return StatusBuilder(::aistreams::StatusCode::kUnimplemented, location);
}

inline StatusBuilder UnknownErrorBuilder(
    ::aistreams::source_location location) {
  return StatusBuilder(::aistreams::StatusCode::kUnknown, location);
}

}  // namespace aistreams

#endif  // AISTREAMS_UTIL_STATUS_STATUS_BUILDER_H_
