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

#include "aistreams/util/status/status_builder.h"

#include "absl/container/flat_hash_map.h"
#include "absl/memory/memory.h"
#include "absl/strings/str_format.h"
#include "absl/synchronization/mutex.h"
#include "aistreams/port/logging.h"
#include "aistreams/util/examine_stack.h"
#include "aistreams/util/status/canonical_errors.h"

namespace aistreams {

namespace {
using ::aistreams::util::CurrentStackTrace;
using ::google::LogMessage;
}  // namespace

StatusBuilder::Rep::Rep(const Rep& r)
    : logging_mode(r.logging_mode),
      log_severity(r.log_severity),
      verbose_level(r.verbose_level),
      n(r.n),
      stream_message(r.stream_message),
      stream(&stream_message),
      should_log_stack_trace(r.should_log_stack_trace),
      message_join_style(r.message_join_style) {}

Status StatusBuilder::JoinMessageToStatus(const Status& s,
                                          absl::string_view msg,
                                          MessageJoinStyle style) {
  if (s.ok() || msg.empty()) return s;

  std::string new_msg;
  if (style == MessageJoinStyle::kAnnotate) {
    std::string formatted_msg =
        absl::StrFormat("[%s:%d] %s", loc_.file_name(), loc_.line(), msg);
    if (!s.message().empty()) {
      new_msg = absl::StrFormat("%s; %s", s.message(), formatted_msg);
    } else {
      new_msg = std::move(formatted_msg);
    }
  } else if (style == MessageJoinStyle::kPrepend) {
    new_msg = absl::StrCat(msg, s.message());
  } else {
    new_msg = absl::StrCat(s.message(), msg);
  }
  return Status(s.code(), new_msg);
}

void StatusBuilder::ConditionallyLog(const Status& result) const {
  if (rep_->logging_mode == Rep::LoggingMode::kDisabled) return;

  int severity = rep_->log_severity;
  switch (rep_->logging_mode) {
    case Rep::LoggingMode::kVLog: {
      if (!VLOG_IS_ON(rep_->verbose_level)) {
        return;
      }
      severity = ::google::INFO;
      break;
    }
    case Rep::LoggingMode::kDisabled:
    case Rep::LoggingMode::kLog:
      break;
    case Rep::LoggingMode::kLogEveryN: {
      {
        struct LogSites {
          absl::Mutex mutex;
          absl::flat_hash_map<std::pair<const void*, uint>, uint>
              counts_by_file_and_line ABSL_GUARDED_BY(mutex);
        };
        static auto* log_every_n_sites = new LogSites();

        log_every_n_sites->mutex.Lock();
        const uint count =
            log_every_n_sites
                ->counts_by_file_and_line[{loc_.file_name(), loc_.line()}]++;
        log_every_n_sites->mutex.Unlock();

        if (count % rep_->n != 0) {
          return;
        }
        break;
      }
    }
  }

  const std::string maybe_stack_trace =
      rep_->should_log_stack_trace ? absl::StrCat(" ", CurrentStackTrace(true))
                                   : "";
  LogMessage(loc_.file_name(), loc_.line(), severity).stream()
      << result << maybe_stack_trace;
}

Status StatusBuilder::CreateStatusAndConditionallyLog() && {
  Status result = JoinMessageToStatus(std::move(status_), rep_->stream_message,
                                      rep_->message_join_style);
  ConditionallyLog(result);

  // We consumed the status above, we set it to some error just to prevent
  // people relying on it become OK or something.
  status_ = UnknownError("");
  rep_ = nullptr;
  return result;
}

std::ostream& operator<<(std::ostream& os, const StatusBuilder& builder) {
  return os << static_cast<Status>(builder);
}

std::ostream& operator<<(std::ostream& os, StatusBuilder&& builder) {
  return os << static_cast<Status>(std::move(builder));
}

}  // namespace aistreams
