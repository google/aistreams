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

#ifndef AISTREAMS_UTIL_STATUS_CANONICAL_ERRORS_H_
#define AISTREAMS_UTIL_STATUS_CANONICAL_ERRORS_H_

#include "aistreams/util/status/status.h"

namespace aistreams {

// Each of the functions below creates a canonical error with the given
// message. The error code of the returned status object matches the name of
// the function.
inline ::aistreams::Status AbortedError(absl::string_view message = "") {
  return ::aistreams::Status(::aistreams::StatusCode::kAborted, message);
}

inline ::aistreams::Status AlreadyExistsError(absl::string_view message = "") {
  return ::aistreams::Status(::aistreams::StatusCode::kAlreadyExists, message);
}

inline ::aistreams::Status CancelledError(absl::string_view message = "") {
  return ::aistreams::Status(::aistreams::StatusCode::kCancelled, message);
}

inline ::aistreams::Status DataLossError(absl::string_view message = "") {
  return ::aistreams::Status(::aistreams::StatusCode::kDataLoss, message);
}

inline ::aistreams::Status DeadlineExceededError(
    absl::string_view message = "") {
  return ::aistreams::Status(::aistreams::StatusCode::kDeadlineExceeded,
                             message);
}

inline ::aistreams::Status FailedPreconditionError(
    absl::string_view message = "") {
  return ::aistreams::Status(::aistreams::StatusCode::kFailedPrecondition,
                             message);
}

inline ::aistreams::Status InternalError(absl::string_view message = "") {
  return ::aistreams::Status(::aistreams::StatusCode::kInternal, message);
}

inline ::aistreams::Status InvalidArgumentError(
    absl::string_view message = "") {
  return ::aistreams::Status(::aistreams::StatusCode::kInvalidArgument,
                             message);
}

inline ::aistreams::Status NotFoundError(absl::string_view message = "") {
  return ::aistreams::Status(::aistreams::StatusCode::kNotFound, message);
}

inline ::aistreams::Status OutOfRangeError(absl::string_view message = "") {
  return ::aistreams::Status(::aistreams::StatusCode::kOutOfRange, message);
}

inline ::aistreams::Status PermissionDeniedError(
    absl::string_view message = "") {
  return ::aistreams::Status(::aistreams::StatusCode::kPermissionDenied,
                             message);
}

inline ::aistreams::Status ResourceExhaustedError(
    absl::string_view message = "") {
  return ::aistreams::Status(::aistreams::StatusCode::kResourceExhausted,
                             message);
}

inline ::aistreams::Status UnauthenticatedError(
    absl::string_view message = "") {
  return ::aistreams::Status(::aistreams::StatusCode::kUnauthenticated,
                             message);
}

inline ::aistreams::Status UnimplementedError(absl::string_view message = "") {
  return ::aistreams::Status(::aistreams::StatusCode::kUnimplemented, message);
}

inline ::aistreams::Status UnknownError(absl::string_view message = "") {
  return ::aistreams::Status(::aistreams::StatusCode::kUnknown, message);
}

inline ::aistreams::Status UnavailableError(absl::string_view message = "") {
  return ::aistreams::Status(::aistreams::StatusCode::kUnavailable, message);
}

inline bool IsAborted(const ::aistreams::Status& status) {
  return status.code() == ::aistreams::StatusCode::kAborted;
}

inline bool IsAlreadyExists(const ::aistreams::Status& status) {
  return status.code() == ::aistreams::StatusCode::kAlreadyExists;
}

inline bool IsCancelled(const ::aistreams::Status& status) {
  return status.code() == ::aistreams::StatusCode::kCancelled;
}

inline bool IsDataLoss(const ::aistreams::Status& status) {
  return status.code() == ::aistreams::StatusCode::kDataLoss;
}

inline bool IsDeadlineExceeded(const ::aistreams::Status& status) {
  return status.code() == ::aistreams::StatusCode::kDeadlineExceeded;
}

inline bool IsFailedPrecondition(const ::aistreams::Status& status) {
  return status.code() == ::aistreams::StatusCode::kFailedPrecondition;
}

inline bool IsInternal(const ::aistreams::Status& status) {
  return status.code() == ::aistreams::StatusCode::kInternal;
}

inline bool IsInvalidArgument(const ::aistreams::Status& status) {
  return status.code() == ::aistreams::StatusCode::kInvalidArgument;
}

inline bool IsNotFound(const ::aistreams::Status& status) {
  return status.code() == ::aistreams::StatusCode::kNotFound;
}

inline bool IsOutOfRange(const ::aistreams::Status& status) {
  return status.code() == ::aistreams::StatusCode::kOutOfRange;
}

inline bool IsPermissionDenied(const ::aistreams::Status& status) {
  return status.code() == ::aistreams::StatusCode::kPermissionDenied;
}

inline bool IsResourceExhausted(const ::aistreams::Status& status) {
  return status.code() == ::aistreams::StatusCode::kResourceExhausted;
}

inline bool IsUnauthenticated(const ::aistreams::Status& status) {
  return status.code() == ::aistreams::StatusCode::kUnauthenticated;
}

inline bool IsUnknown(const ::aistreams::Status& status) {
  return status.code() == ::aistreams::StatusCode::kUnknown;
}

inline bool IsUnavailable(const ::aistreams::Status& status) {
  return status.code() == ::aistreams::StatusCode::kUnavailable;
}

inline bool IsUnimplemented(const ::aistreams::Status& status) {
  return status.code() == ::aistreams::StatusCode::kUnimplemented;
}

}  // namespace aistreams

#endif  // AISTREAMS_UTIL_STATUS_CANONICAL_ERRORS_H_
