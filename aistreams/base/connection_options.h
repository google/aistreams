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

namespace aistreams {
namespace base {

// Options to enable/configure SSL.
struct SslOptions {
  // Use an insecure channel to connect to the server.
  //
  // If false, then you have to review and the fields below as appropriate.
  bool use_insecure_channel = true;

  // The expected ssl domain name of the server.
  std::string ssl_domain_name;

  // This file path to the root CA certificate.
  //
  // TODO(yxyan, dschao): Look into ways to get this from a CA authority
  // rather than having to pre-distribute copies.
  std::string ssl_root_cert_path;
};

// Options to connect to a service in AI Streams.
struct ConnectionOptions {
  // This is the ip:port to an AI Streams service.
  //
  // Most commonly, this is the endpoint of the ingress.
  std::string target_address;

  // Options to enable/configure SSL.
  SslOptions ssl_options;
};

}  // namespace base
}  // namespace aistreams

#endif  // AISTREAMS_BASE_CONNECTION_OPTIONS_H_
