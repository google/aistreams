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

#include "aistreams/c/c_api.h"

#include <string>

#include "aistreams/base/connection_options.h"
#include "aistreams/c/c_api_internal.h"
#include "aistreams/port/gtest.h"
#include "aistreams/port/logging.h"
#include "aistreams/port/status.h"
#include "aistreams/port/statusor.h"

namespace aistreams {

TEST(CAPI, ConnectionOptionsTest) {
  AIS_ConnectionOptions* ais_options = AIS_NewConnectionOptions();
  ConnectionOptions* options = &ais_options->connection_options;

  AIS_SetTargetAddress("localhost:50051", ais_options);
  EXPECT_EQ(options->target_address, "localhost:50051");

  AIS_SetAuthenticateWithGoogle(0, ais_options);
  EXPECT_FALSE(options->authenticate_with_google);
  AIS_SetAuthenticateWithGoogle(1, ais_options);
  EXPECT_TRUE(options->authenticate_with_google);

  AIS_SetUseInsecureChannel(0, ais_options);
  EXPECT_FALSE(options->ssl_options.use_insecure_channel);
  AIS_SetUseInsecureChannel(1, ais_options);
  EXPECT_TRUE(options->ssl_options.use_insecure_channel);

  AIS_SetSslDomainName("aistreams.io", ais_options);
  EXPECT_EQ(options->ssl_options.ssl_domain_name, "aistreams.io");

  AIS_SetSslRootCertPath("/some/fake/path", ais_options);
  EXPECT_EQ(options->ssl_options.ssl_root_cert_path, "/some/fake/path");

  AIS_DeleteConnectionOptions(ais_options);
}

}  // namespace aistreams
