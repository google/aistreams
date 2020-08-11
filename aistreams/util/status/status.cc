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

#include "aistreams/util/status/status.h"

#include <stdio.h>

namespace aistreams {

// Port the implementation from absl
// https://github.com/abseil/abseil-cpp/blob/master/absl/status/status.cc#L30
// Remove the method after absl status is imported.
std::string StatusCodeToString(StatusCode code) {
  switch (code) {
    case ::aistreams::StatusCode::kOk:
      return "OK";
    case ::aistreams::StatusCode::kCancelled:
      return "Cancelled";
    case ::aistreams::StatusCode::kUnknown:
      return "Unknown";
    case ::aistreams::StatusCode::kInvalidArgument:
      return "Invalid argument";
    case ::aistreams::StatusCode::kDeadlineExceeded:
      return "Deadline exceeded";
    case ::aistreams::StatusCode::kNotFound:
      return "Not found";
    case ::aistreams::StatusCode::kAlreadyExists:
      return "Already exists";
    case ::aistreams::StatusCode::kPermissionDenied:
      return "Permission denied";
    case ::aistreams::StatusCode::kUnauthenticated:
      return "Unauthenticated";
    case ::aistreams::StatusCode::kResourceExhausted:
      return "Resource exhausted";
    case ::aistreams::StatusCode::kFailedPrecondition:
      return "Failed precondition";
    case ::aistreams::StatusCode::kAborted:
      return "Aborted";
    case ::aistreams::StatusCode::kOutOfRange:
      return "Out of range";
    case ::aistreams::StatusCode::kUnimplemented:
      return "Unimplemented";
    case ::aistreams::StatusCode::kInternal:
      return "Internal";
    case ::aistreams::StatusCode::kUnavailable:
      return "Unavailable";
    case ::aistreams::StatusCode::kDataLoss:
      return "Data loss";
    default:
      return "";
  }
}

std::ostream& operator<<(std::ostream& os, StatusCode code) {
  return os << StatusCodeToString(code);
}

Status::Status(::aistreams::StatusCode code, absl::string_view msg) {
  state_ = std::unique_ptr<State>(new State);
  state_->code = code;
  state_->msg = std::string(msg);
}

void Status::Update(const Status& new_status) {
  if (ok()) {
    *this = new_status;
  }
}

void Status::SlowCopyFrom(const State* src) {
  if (src == nullptr) {
    state_ = nullptr;
  } else {
    state_ = std::unique_ptr<State>(new State(*src));
  }
}

const std::string& Status::empty_string() {
  static std::string* empty = new std::string;
  return *empty;
}

std::string Status::ToString() const {
  if (state_ == nullptr) {
    return "OK";
  } else {
    char tmp[30];
    const char* type;
    switch (code()) {
      case ::aistreams::StatusCode::kCancelled:
        type = "Cancelled";
        break;
      case ::aistreams::StatusCode::kUnknown:
        type = "Unknown";
        break;
      case ::aistreams::StatusCode::kInvalidArgument:
        type = "Invalid argument";
        break;
      case ::aistreams::StatusCode::kDeadlineExceeded:
        type = "Deadline exceeded";
        break;
      case ::aistreams::StatusCode::kNotFound:
        type = "Not found";
        break;
      case ::aistreams::StatusCode::kAlreadyExists:
        type = "Already exists";
        break;
      case ::aistreams::StatusCode::kPermissionDenied:
        type = "Permission denied";
        break;
      case ::aistreams::StatusCode::kUnauthenticated:
        type = "Unauthenticated";
        break;
      case ::aistreams::StatusCode::kResourceExhausted:
        type = "Resource exhausted";
        break;
      case ::aistreams::StatusCode::kFailedPrecondition:
        type = "Failed precondition";
        break;
      case ::aistreams::StatusCode::kAborted:
        type = "Aborted";
        break;
      case ::aistreams::StatusCode::kOutOfRange:
        type = "Out of range";
        break;
      case ::aistreams::StatusCode::kUnimplemented:
        type = "Unimplemented";
        break;
      case ::aistreams::StatusCode::kInternal:
        type = "Internal";
        break;
      case ::aistreams::StatusCode::kUnavailable:
        type = "Unavailable";
        break;
      case ::aistreams::StatusCode::kDataLoss:
        type = "Data loss";
        break;
      default:
        snprintf(tmp, sizeof(tmp), "Unknown code(%d)",
                 static_cast<int>(code()));
        type = tmp;
        break;
    }
    std::string result(type);
    result += ": ";
    result += state_->msg;
    return result;
  }
}

void Status::IgnoreError() const {
  // no-op
}

std::ostream& operator<<(std::ostream& os, const Status& x) {
  os << x.ToString();
  return os;
}

std::string* CheckOpHelperOutOfLine(const ::aistreams::Status& v,
                                    const char* msg) {
  std::string r("Non-OK-status: ");
  r += msg;
  r += " status: ";
  r += v.ToString();
  // Leaks std::string but this is only to be used in a fatal error message
  return new std::string(r);
}

}  // namespace aistreams
