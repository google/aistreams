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

#ifndef AISTREAMS_BASE_UTIL_AUTH_HELPERS_H_
#define AISTREAMS_BASE_UTIL_AUTH_HELPERS_H_

#include "aistreams/port/statusor.h"

namespace aistreams {

// GetIdToken will send request to IAM service to generate a ID token. You
// need to set the GOOGLE_APPLICATION_CREDENTIALS to the file path of the JSON
// file that contains your service account key. The service account needs to
// have the Service Account Token Creator role
// (roles/iam.serviceAccountTokenCreator).
StatusOr<std::string> GetIdToken(const std::string& service_account);

// GetIdTokenWithDefaultServiceAccount will use the service account from the
// JSON key file.You need to set the GOOGLE_APPLICATION_CREDENTIALS to the file
// path of the JSON file that contains your service account key. The service
// account needs to have the Service Account Token Creator role
// (roles/iam.serviceAccountTokenCreator).
StatusOr<std::string> GetIdTokenWithDefaultServiceAccount();

}  // namespace aistreams

#endif  // AISTREAMS_BASE_UTIL_AUTH_HELPERS_H_