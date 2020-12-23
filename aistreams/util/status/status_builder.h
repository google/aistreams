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
#include "absl/strings/internal/ostringstream.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/time/time.h"
#include "aistreams/port/logging.h"
#include "aistreams/util/status/source_location.h"
#include "aistreams/util/status/status.h"

namespace aistreams {

// Creates a status based on an original_status, but enriched with additional
// information.  The builder implicitly converts to Status and StatusOr<T>
// allowing for it to be returned directly.
//
//   StatusBuilder builder(original);
//   util::AttachPayload(&builder, proto);
//   builder << "info about error";
//   return builder;
//
// It provides method chaining to simplify typical usage:
//
//   return StatusBuilder(original)
//       .Log(base_logging::WARNING) << "oh no!";
//
// In more detail:
// - When the original status is OK, all methods become no-ops and nothing will
//   be logged.
// - Messages streamed into the status builder are collected into a single
//   additional message string.
// - The original Status's message and the additional message are joined
//   together when the result status is built.
// - By default, the messages will be joined as if by `util::Annotate`, which
//   includes a convenience separator between the original message and the
//   additional one.  This behavior can be changed with the `SetAppend()` and
//   `SetPrepend()` methods of the builder.
// - By default, the result status is not logged.  The `Log` and
//   `EmitStackTrace` methods will cause the builder to log the result status
//   when it is built.
// - All side effects (like logging or constructing a stack trace) happen when
//   the builder is converted to a status.
class ABSL_MUST_USE_RESULT StatusBuilder {
 public:
  // Creates a `StatusBuilder` based on an original status.  If logging is
  // enabled, it will use `location` as the location from which the log message
  // occurs.  A typical user will not specify `location`, allowing it to default
  // to the current location.
  explicit StatusBuilder(const Status& original_status,
                         SourceLocation location AIS_LOC_CURRENT_DEFAULT_ARG);
  explicit StatusBuilder(Status&& original_status,
                         SourceLocation location AIS_LOC_CURRENT_DEFAULT_ARG);

  // Creates a `StatusBuilder` from a status code. If logging is enabled, it
  // will use `location` as the location from which the log message occurs.  A
  // typical user will not specify `location`, allowing it to default to the
  // current location.
  explicit StatusBuilder(StatusCode code,
                         SourceLocation location AIS_LOC_CURRENT_DEFAULT_ARG);

  StatusBuilder(const StatusBuilder& sb);
  StatusBuilder& operator=(const StatusBuilder& sb);
  StatusBuilder(StatusBuilder&&) = default;
  StatusBuilder& operator=(StatusBuilder&&) = default;

  // Mutates the builder so that the final additional message is prepended to
  // the original error message in the status.  A convenience separator is not
  // placed between the messages.
  //
  // NOTE: Multiple calls to `SetPrepend` and `SetAppend` just adjust the
  // behavior of the final join of the original status with the extra message.
  //
  // Returns `*this` to allow method chaining.
  StatusBuilder& SetPrepend() &;
  ABSL_MUST_USE_RESULT StatusBuilder&& SetPrepend() &&;

  // Mutates the builder so that the final additional message is prepended to
  // the original error message in the status.  A convenience separator is not
  // placed between the messages.
  //
  // NOTE: Multiple calls to `SetPrepend` and `SetAppend` just adjust the
  // behavior of the final join of the original status with the extra message.
  //
  // Returns `*this` to allow method chaining.
  StatusBuilder& SetAppend() &;
  ABSL_MUST_USE_RESULT StatusBuilder&& SetAppend() &&;

  // Mutates the builder to disable any logging that was set using any of the
  // logging functions below.  Returns `*this` to allow method chaining.
  StatusBuilder& SetNoLogging() &;
  ABSL_MUST_USE_RESULT StatusBuilder&& SetNoLogging() &&;

  // Mutates the builder so that the result status will be logged (without a
  // stack trace) when this builder is converted to a Status.  This overrides
  // the logging settings from earlier calls to any of the logging mutator
  // functions.  Returns `*this` to allow method chaining.
  StatusBuilder& Log(::google::LogSeverity level) &;
  ABSL_MUST_USE_RESULT StatusBuilder&& Log(::google::LogSeverity level) &&;
  StatusBuilder& LogError() & { return Log(::google::ERROR); }
  ABSL_MUST_USE_RESULT StatusBuilder&& LogError() && {
    return std::move(LogError());
  }
  StatusBuilder& LogWarning() & { return Log(::google::WARNING); }
  ABSL_MUST_USE_RESULT StatusBuilder&& LogWarning() && {
    return std::move(LogWarning());
  }
  StatusBuilder& LogInfo() & { return Log(::google::INFO); }
  ABSL_MUST_USE_RESULT StatusBuilder&& LogInfo() && {
    return std::move(LogInfo());
  }

  // Mutates the builder so that the result status will be logged every N
  // invocations (without a stack trace) when this builder is converted to a
  // Status.  This overrides the logging settings from earlier calls to any of
  // the logging mutator functions.  Returns `*this` to allow method chaining.
  StatusBuilder& LogEveryN(::google::LogSeverity level, int n) &;
  ABSL_MUST_USE_RESULT StatusBuilder&& LogEveryN(::google::LogSeverity level,
                                                 int n) &&;

  // Mutates the builder so that the result status will be VLOGged (without a
  // stack trace) when this builder is converted to a Status.  `verbose_level`
  // indicates the verbosity level that would be passed to VLOG().  This
  // overrides the logging settings from earlier calls to any of the logging
  // mutator functions.  Returns `*this` to allow method chaining.
  StatusBuilder& VLog(int verbose_level) &;
  ABSL_MUST_USE_RESULT StatusBuilder&& VLog(int verbose_level) &&;

  // Mutates the builder so that a stack trace will be logged if the status is
  // logged. One of the logging setters above should be called as well. If
  // logging is not yet enabled this behaves as if LogInfo().EmitStackTrace()
  // was called. Returns `*this` to allow method chaining.
  StatusBuilder& EmitStackTrace() &;
  ABSL_MUST_USE_RESULT StatusBuilder&& EmitStackTrace() &&;

  // Appends to the extra message that will be added to the original status. By
  // default, the extra message is added to the original message as if by
  // `util::Annotate`, which includes a convenience separator between the
  // original message and the enriched one.
  template <typename T>
  StatusBuilder& operator<<(const T& value) &;
  template <typename T>
  ABSL_MUST_USE_RESULT StatusBuilder&& operator<<(const T& value) &&;

  // Sets the status code for the status that will be returned by this
  // StatusBuilder. Returns `*this` to allow method chaining.
  StatusBuilder& SetCode(StatusCode code) &;
  ABSL_MUST_USE_RESULT StatusBuilder&& SetCode(StatusCode code) &&;

  // Calls `adaptor` on this status builder to apply policies, type conversions,
  // and/or side effects on the StatusBuilder. Returns the value returned by
  // `adaptor`, which may be any type including `void`.
  template <typename Adaptor>
  auto With(Adaptor&& adaptor) & -> decltype(
      std::forward<Adaptor>(adaptor)(*this)) {
    return std::forward<Adaptor>(adaptor)(*this);
  }
  template <typename Adaptor>
  ABSL_MUST_USE_RESULT auto With(Adaptor&& adaptor) && -> decltype(
      std::forward<Adaptor>(adaptor)(std::move(*this))) {
    return std::forward<Adaptor>(adaptor)(std::move(*this));
  }

  // Returns true if the Status created by this builder will be ok().
  ABSL_MUST_USE_RESULT bool ok() const;

  // Returns the (canonical) error code for the Status created by this builder.
  ABSL_MUST_USE_RESULT StatusCode code() const;

  // Implicit conversion to Status.
  //
  // Careful: this operator has side effects, so it should be called at
  // most once. In particular, do NOT use this conversion operator to inspect
  // the status from an adapter object passed into With().
  operator Status() const&;
  operator Status() &&;

  // Returns the source location used to create this builder.
  ABSL_MUST_USE_RESULT SourceLocation source_location() const;

 private:
  // Specifies how to join the error message in the original status and any
  // additional message that has been streamed into the builder.
  enum class MessageJoinStyle {
    kAnnotate,
    kAppend,
    kPrepend,
  };

  // Creates a new status based on an old one by joining the message from the
  // original to an additional message.
  Status JoinMessageToStatus(const Status& s, absl::string_view msg,
                             MessageJoinStyle style);

  // Creates a Status from this builder and logs it if the builder has been
  // configured to log itself.
  Status CreateStatusAndConditionallyLog() &&;

  // Conditionally logs if the builder has been configured to log.  This method
  // is split from the above to isolate the portability issues around logging
  // into a single place.
  void ConditionallyLog(const Status& status) const;

  // Infrequently set builder options, instantiated lazily. This reduces
  // average construction/destruction time (e.g. the `stream` is fairly
  // expensive). Stacks can also be blown if StatusBuilder grows too large.
  // This is primarily an issue for debug builds, which do not necessarily
  // re-use stack space within a function across the sub-scopes used by
  // status macros.
  struct Rep {
    explicit Rep() = default;
    Rep(const Rep& r);

    enum class LoggingMode {
      kDisabled,
      kLog,
      kVLog,
      kLogEveryN,
    };
    LoggingMode logging_mode = LoggingMode::kDisabled;

    // Corresponds to the levels in `base_logging::LogSeverity`.
    // `logging_mode == LoggingMode::kVLog` always logs at severity INFO.
    int log_severity;

    // The level at which the Status should be VLOGged.
    // Only used when `logging_mode == LoggingMode::kVLog`.
    int verbose_level;

    // Only log every N invocations.
    // Only used when `logging_mode == LoggingMode::kLogEveryN`.
    int n;

    // Gathers additional messages added with `<<` for use in the final status.
    std::string stream_message;
    absl::strings_internal::OStringStream stream{&stream_message};

    // Whether to log stack trace.  Only used when `logging_mode !=
    // LoggingMode::kDisabled`.
    bool should_log_stack_trace = false;

    // Specifies how to join the message in `status_` and `stream`.
    MessageJoinStyle message_join_style = MessageJoinStyle::kAnnotate;
  };

  // The status that the result will be based on.  Can be modified by
  // util::AttachPayload().
  Status status_;

  // The location to record if this status is logged.
  SourceLocation loc_;

  // nullptr if the result status will be OK.  Extra fields moved to the heap
  // to minimize stack space.
  std::unique_ptr<Rep> rep_;
};

// Implicitly converts `builder` to `Status` and write it to `os`.
std::ostream& operator<<(std::ostream& os, const StatusBuilder& builder);
std::ostream& operator<<(std::ostream& os, StatusBuilder&& builder);

inline StatusBuilder AbortedErrorBuilder(
    ::aistreams::SourceLocation location AIS_LOC_CURRENT_DEFAULT_ARG) {
  return StatusBuilder(::aistreams::StatusCode::kAborted, location);
}

inline StatusBuilder AlreadyExistsErrorBuilder(
    ::aistreams::SourceLocation location AIS_LOC_CURRENT_DEFAULT_ARG) {
  return StatusBuilder(::aistreams::StatusCode::kAlreadyExists, location);
}

inline StatusBuilder CancelledErrorBuilder(
    ::aistreams::SourceLocation location AIS_LOC_CURRENT_DEFAULT_ARG) {
  return StatusBuilder(::aistreams::StatusCode::kCancelled, location);
}

inline StatusBuilder DataLossErrorBuilder(
    ::aistreams::SourceLocation location AIS_LOC_CURRENT_DEFAULT_ARG) {
  return StatusBuilder(::aistreams::StatusCode::kDataLoss, location);
}

inline StatusBuilder DeadlineExceededErrorBuilder(
    ::aistreams::SourceLocation location AIS_LOC_CURRENT_DEFAULT_ARG) {
  return StatusBuilder(::aistreams::StatusCode::kDeadlineExceeded, location);
}

inline StatusBuilder FailedPreconditionErrorBuilder(
    ::aistreams::SourceLocation location AIS_LOC_CURRENT_DEFAULT_ARG) {
  return StatusBuilder(::aistreams::StatusCode::kFailedPrecondition, location);
}

inline StatusBuilder InternalErrorBuilder(
    ::aistreams::SourceLocation location AIS_LOC_CURRENT_DEFAULT_ARG) {
  return StatusBuilder(::aistreams::StatusCode::kInternal, location);
}

inline StatusBuilder InvalidArgumentErrorBuilder(
    ::aistreams::SourceLocation location AIS_LOC_CURRENT_DEFAULT_ARG) {
  return StatusBuilder(::aistreams::StatusCode::kInvalidArgument, location);
}

inline StatusBuilder NotFoundErrorBuilder(
    ::aistreams::SourceLocation location AIS_LOC_CURRENT_DEFAULT_ARG) {
  return StatusBuilder(::aistreams::StatusCode::kNotFound, location);
}

inline StatusBuilder OutOfRangeErrorBuilder(
    ::aistreams::SourceLocation location AIS_LOC_CURRENT_DEFAULT_ARG) {
  return StatusBuilder(::aistreams::StatusCode::kOutOfRange, location);
}

inline StatusBuilder PermissionDeniedErrorBuilder(
    ::aistreams::SourceLocation location AIS_LOC_CURRENT_DEFAULT_ARG) {
  return StatusBuilder(::aistreams::StatusCode::kPermissionDenied, location);
}

inline StatusBuilder ResourceExhaustedErrorBuilder(
    ::aistreams::SourceLocation location AIS_LOC_CURRENT_DEFAULT_ARG) {
  return StatusBuilder(::aistreams::StatusCode::kResourceExhausted, location);
}

inline StatusBuilder UnauthenticatedErrorBuilder(
    ::aistreams::SourceLocation location AIS_LOC_CURRENT_DEFAULT_ARG) {
  return StatusBuilder(::aistreams::StatusCode::kUnauthenticated, location);
}

inline StatusBuilder UnavailableErrorBuilder(
    ::aistreams::SourceLocation location AIS_LOC_CURRENT_DEFAULT_ARG) {
  return StatusBuilder(::aistreams::StatusCode::kUnavailable, location);
}

inline StatusBuilder UnimplementedErrorBuilder(
    ::aistreams::SourceLocation location AIS_LOC_CURRENT_DEFAULT_ARG) {
  return StatusBuilder(::aistreams::StatusCode::kUnimplemented, location);
}

inline StatusBuilder UnknownErrorBuilder(
    ::aistreams::SourceLocation location AIS_LOC_CURRENT_DEFAULT_ARG) {
  return StatusBuilder(::aistreams::StatusCode::kUnknown, location);
}

// StatusBuilder policy to append an extra message to the original status.
//
// This is most useful with adaptors such as util::TaskReturn that otherwise
// would prevent use of operator<<.  For example:
//
//   AIS_RETURN_IF_ERROR(foo(val))
//       .With(util::ExtraMessage("when calling foo()"))
//       .With(util::TaskReturn(task));
//
// or
//
//   AIS_RETURN_IF_ERROR(foo(val))
//       .With(util::ExtraMessage() << "val: " << val)
//       .With(util::TaskReturn(task));
//
// Note in the above example, the RETURN_IF_ERROR macro ensures the ExtraMessage
// expression is evaluated only in the error case, so efficiency of constructing
// the message is not a concern in the success case.
class ExtraMessage {
 public:
  ExtraMessage() : ExtraMessage(std::string()) {}
  explicit ExtraMessage(std::string msg)
      : msg_(std::move(msg)), stream_(&msg_) {}

  // Appends to the extra message that will be added to the original status.  By
  // default, the extra message is added to the original message as if by
  // `util::Annotate`, which includes a convenience separator between the
  // original message and the enriched one.
  template <typename T>
  ExtraMessage& operator<<(const T& value) {
    stream_ << value;
    return *this;
  }

  // Appends to the extra message that will be added to the original status.  By
  // default, the extra message is added to the original message as if by
  // `util::Annotate`, which includes a convenience separator between the
  // original message and the enriched one.
  StatusBuilder operator()(StatusBuilder builder) const {
    builder << msg_;
    return builder;
  }

 private:
  std::string msg_;
  absl::strings_internal::OStringStream stream_;
};

// Implementation details follow; clients should ignore.

inline StatusBuilder::StatusBuilder(const Status& original_status,
                                    SourceLocation location)
    : status_(original_status), loc_(location) {}

inline StatusBuilder::StatusBuilder(Status&& original_status,
                                    SourceLocation location)
    : status_(std::move(original_status)), loc_(location) {}

inline StatusBuilder::StatusBuilder(StatusCode code, SourceLocation location)
    : status_(code, ""), loc_(location) {}

inline StatusBuilder::StatusBuilder(const StatusBuilder& sb)
    : status_(sb.status_), loc_(sb.loc_) {
  if (sb.rep_ != nullptr) {
    rep_ = std::make_unique<Rep>(*sb.rep_);
  }
}

inline StatusBuilder& StatusBuilder::operator=(const StatusBuilder& sb) {
  status_ = sb.status_;
  loc_ = sb.loc_;
  if (sb.rep_ != nullptr) {
    rep_ = std::make_unique<Rep>(*sb.rep_);
  } else {
    rep_ = nullptr;
  }
  return *this;
}

inline StatusBuilder& StatusBuilder::SetPrepend() & {
  if (status_.ok()) return *this;
  if (rep_ == nullptr) rep_ = std::make_unique<Rep>();

  rep_->message_join_style = MessageJoinStyle::kPrepend;
  return *this;
}
inline StatusBuilder&& StatusBuilder::SetPrepend() && {
  return std::move(SetPrepend());
}

inline StatusBuilder& StatusBuilder::SetAppend() & {
  if (status_.ok()) return *this;
  if (rep_ == nullptr) rep_ = std::make_unique<Rep>();
  rep_->message_join_style = MessageJoinStyle::kAppend;
  return *this;
}
inline StatusBuilder&& StatusBuilder::SetAppend() && {
  return std::move(SetAppend());
}

inline StatusBuilder& StatusBuilder::SetNoLogging() & {
  if (rep_ != nullptr) {
    rep_->logging_mode = Rep::LoggingMode::kDisabled;
    rep_->should_log_stack_trace = false;
  }
  return *this;
}
inline StatusBuilder&& StatusBuilder::SetNoLogging() && {
  return std::move(SetNoLogging());
}

inline StatusBuilder& StatusBuilder::Log(::google::LogSeverity level) & {
  if (status_.ok()) return *this;
  if (rep_ == nullptr) rep_ = std::make_unique<Rep>();
  rep_->logging_mode = Rep::LoggingMode::kLog;
  rep_->log_severity = level;
  return *this;
}
inline StatusBuilder&& StatusBuilder::Log(::google::LogSeverity level) && {
  return std::move(Log(level));
}

inline StatusBuilder& StatusBuilder::LogEveryN(::google::LogSeverity level,
                                               int n) & {
  if (status_.ok()) return *this;
  if (n < 1) return Log(level);
  if (rep_ == nullptr) rep_ = std::make_unique<Rep>();
  rep_->logging_mode = Rep::LoggingMode::kLogEveryN;
  rep_->log_severity = level;
  rep_->n = n;
  return *this;
}
inline StatusBuilder&& StatusBuilder::LogEveryN(::google::LogSeverity level,
                                                int n) && {
  return std::move(LogEveryN(level, n));
}

inline StatusBuilder& StatusBuilder::VLog(int verbose_level) & {
  if (status_.ok()) return *this;
  if (rep_ == nullptr) rep_ = std::make_unique<Rep>();
  rep_->logging_mode = Rep::LoggingMode::kVLog;
  rep_->verbose_level = verbose_level;
  return *this;
}
inline StatusBuilder&& StatusBuilder::VLog(int verbose_level) && {
  return std::move(VLog(verbose_level));
}

inline StatusBuilder& StatusBuilder::EmitStackTrace() & {
  if (status_.ok()) return *this;
  if (rep_ == nullptr) {
    rep_ = std::make_unique<Rep>();
    // Default to INFO logging, otherwise nothing would be emitted.
    rep_->logging_mode = Rep::LoggingMode::kLog;
    rep_->log_severity = ::google::INFO;
  }
  rep_->should_log_stack_trace = true;
  return *this;
}
inline StatusBuilder&& StatusBuilder::EmitStackTrace() && {
  return std::move(EmitStackTrace());
}

template <typename T>
StatusBuilder& StatusBuilder::operator<<(const T& value) & {
  if (status_.ok()) return *this;
  if (rep_ == nullptr) rep_ = std::make_unique<Rep>();
  rep_->stream << value;
  return *this;
}
template <typename T>
StatusBuilder&& StatusBuilder::operator<<(const T& value) && {
  return std::move(operator<<(value));
}

inline StatusBuilder& StatusBuilder::SetCode(StatusCode code) & {
  status_ = Status(code, status_.message());
  return *this;
}
inline StatusBuilder&& StatusBuilder::SetCode(StatusCode code) && {
  return std::move(SetCode(code));
}

inline bool StatusBuilder::ok() const { return status_.ok(); }

inline StatusCode StatusBuilder::code() const { return status_.code(); }

inline StatusBuilder::operator Status() const& {
  if (rep_ == nullptr) return status_;
  return StatusBuilder(*this).CreateStatusAndConditionallyLog();
}
inline StatusBuilder::operator Status() && {
  if (rep_ == nullptr) return std::move(status_);
  return std::move(*this).CreateStatusAndConditionallyLog();
}

inline SourceLocation StatusBuilder::source_location() const { return loc_; }

}  // namespace aistreams

#endif  // AISTREAMS_UTIL_STATUS_STATUS_BUILDER_H_
