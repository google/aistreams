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

#include "aistreams/gstreamer/gst-plugins/cli_builders/aissrc_cli_builder.h"

#include <string>

#include "absl/strings/str_format.h"
#include "absl/strings/str_join.h"
#include "absl/strings/string_view.h"
#include "aistreams/port/canonical_errors.h"
#include "aistreams/port/logging.h"
#include "aistreams/port/status.h"
#include "aistreams/port/status_macros.h"
#include "aistreams/port/statusor.h"

namespace aistreams {

namespace {

std::string ToString(bool b) { return b ? "true" : "false"; }

std::string SetPluginParam(absl::string_view parameter_name,
                           absl::string_view value) {
  if (value.empty()) {
    value = "\"\"";
  }
  return absl::StrFormat("%s=%s", parameter_name, value);
}

}  // namespace

Status AissrcCliBuilder::ValidateSettings() const {
  if (target_address_.empty()) {
    return InvalidArgumentError("Given an empty target address");
  }
  if (stream_name_.empty()) {
    return InvalidArgumentError("Given an empty stream name");
  }
  if (!use_insecure_channel_) {
    if (ssl_domain_name_.empty()) {
      return InvalidArgumentError("Given an empty ssl domain name");
    }
    if (ssl_root_cert_path_.empty()) {
      return InvalidArgumentError("Given an empty path to the ssl root cert");
    }
  }
  return OkStatus();
}

StatusOr<std::string> AissrcCliBuilder::Finalize() const {
  AIS_RETURN_IF_ERROR(ValidateSettings());
  std::vector<std::string> tokens;
  tokens.push_back("aissrc");
  tokens.push_back(SetPluginParam("target-address", target_address_));
  tokens.push_back(SetPluginParam("authenticate-with-google",
                                  ToString(authenticate_with_google_)));
  tokens.push_back(SetPluginParam("stream-name", stream_name_));
  tokens.push_back(
      SetPluginParam("use-insecure-channel", ToString(use_insecure_channel_)));
  tokens.push_back(SetPluginParam("ssl-domain-name", ssl_domain_name_));
  tokens.push_back(SetPluginParam("ssl-root-cert-path", ssl_root_cert_path_));
  tokens.push_back(
      SetPluginParam("timeout-in-sec", std::to_string(timeout_in_sec_)));
  return absl::StrJoin(tokens, " ");
}

}  // namespace aistreams
