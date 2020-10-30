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

#ifndef AISTREAMS_GSTREAMER_GST_PLUGINS_CLI_BUILDERS_AISSINK_CLI_BUILDER_H_
#define AISTREAMS_GSTREAMER_GST_PLUGINS_CLI_BUILDERS_AISSINK_CLI_BUILDER_H_

#include <string>

#include "absl/strings/str_format.h"
#include "absl/strings/str_join.h"
#include "absl/strings/string_view.h"
#include "aistreams/base/connection_options.h"
#include "aistreams/port/logging.h"
#include "aistreams/port/status.h"
#include "aistreams/port/statusor.h"

namespace aistreams {

// Builder class to build the cli configurations for aissink.
class AissinkCliBuilder {
 public:
  AissinkCliBuilder() = default;

  AissinkCliBuilder& SetTargetAddress(const std::string& target_address) {
    target_address_ = target_address;
    return *this;
  }

  AissinkCliBuilder& SetAuthenticateWithGoogle(bool authenticate_with_google) {
    authenticate_with_google_ = authenticate_with_google;
    return *this;
  }

  AissinkCliBuilder& SetStreamName(const std::string& stream_name) {
    stream_name_ = stream_name;
    return *this;
  }

  AissinkCliBuilder& SetSslOptions(const SslOptions& options) {
    use_insecure_channel_ = options.use_insecure_channel;
    if (!use_insecure_channel_) {
      ssl_domain_name_ = options.ssl_domain_name;
      ssl_root_cert_path_ = options.ssl_root_cert_path;
    }
    return *this;
  }

  AissinkCliBuilder& SetTraceProbability(double trace_probability) {
    trace_probability_ = trace_probability;
    return *this;
  }

  // On success, returns the gstreamer commandline configuration string.
  StatusOr<std::string> Finalize() const;

 private:
  Status ValidateSettings() const;

  std::string target_address_;
  bool authenticate_with_google_;
  std::string stream_name_;

  bool use_insecure_channel_;
  std::string ssl_domain_name_;
  std::string ssl_root_cert_path_;

  double trace_probability_ = 0;
};

}  // namespace aistreams

#endif  // AISTREAMS_GSTREAMER_GST_PLUGINS_CLI_BUILDERS_AISSINK_CLI_BUILDER_H_
