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

#ifndef AISTREAMS_BASE_CONNECTION_OPTIONS_H_
#define AISTREAMS_BASE_CONNECTION_OPTIONS_H_

#include <string>

#include "absl/time/time.h"

namespace aistreams {

// Options to enable/configure SSL.
struct SslOptions {
  // Use an insecure channel to connect to the server.
  //
  // If false, then you have to review and the fields below as appropriate.
  bool use_insecure_channel = false;

  // The expected ssl domain name of the server.
  std::string ssl_domain_name = "aistreams.googleapis.com";

  // This file path to the root CA certificate.
  std::string ssl_root_cert_path;
};

// Options to configure a RPCs.
struct RpcOptions {
  // The timeout for a call. Set a positive value to timeout. By default its
  // value is infinite long, and won't be used for RPC calls.
  absl::Duration timeout = absl::InfiniteDuration();

  // If true, then block until the underlying communication channel
  // becomes ready instead of failing fast.
  bool wait_for_ready = false;
};

// AI Streams connection options.
//
// There are two modes of AI Streams deployment: onprem or google managed.
// You may need to set the options below differently depending on which you are
// using.
//
// TODO: The onprem/managed separation should be clearer and easier to use.
// Please come back to clean this up.
struct ConnectionOptions {
  // ------------------------------------------------------------------------
  // General options

  // Address to the AI Streams service.
  //
  // For control plane operations in the google managed service (e.g. cluster,
  // stream creation, deletion, list, etc.), set this to
  // aistreams.googleapis.com.
  //
  // For data plane operations in the google managed service (e.g. send/receive
  // packets) and all operations in the onprem service, set this to the ip:port
  // of the k8s Ingress.
  std::string target_address;

  // Set this to false for onprem; true for google managed.
  bool authenticate_with_google = false;

  // ------------------------------------------------------------------------
  // Options for the k8s Ingress
  //
  // For onprem, these options are used for both control and data planes
  // operations. For the managed service, these options are used only for data
  // plane operations.

  // Options to configure TLS/SSL.
  SslOptions ssl_options;

  // Options to configure RPCs.
  RpcOptions rpc_options;
};

}  // namespace aistreams

#endif  // AISTREAMS_BASE_CONNECTION_OPTIONS_H_
