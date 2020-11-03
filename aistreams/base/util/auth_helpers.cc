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

#undef RAPIDJSON_HAS_STDSTRING
#define RAPIDJSON_HAS_STDSTRING 1

#include "aistreams/base/util/auth_helpers.h"

#include "absl/strings/str_format.h"
#include "absl/strings/strip.h"
#include "aistreams/base/util/grpc_helpers.h"
#include "aistreams/port/canonical_errors.h"
#include "aistreams/util/file_helpers.h"
#include "google/iam/credentials/v1/common.grpc.pb.h"
#include "google/iam/credentials/v1/common.pb.h"
#include "google/iam/credentials/v1/iamcredentials.grpc.pb.h"
#include "google/iam/credentials/v1/iamcredentials.pb.h"
#include "rapidjson/document.h"

namespace aistreams {
namespace {
using ::google::iam::credentials::v1::GenerateIdTokenRequest;
using ::google::iam::credentials::v1::GenerateIdTokenResponse;
using ::google::iam::credentials::v1::IAMCredentials;

constexpr char kIAMGoogleAPI[] = "iamcredentials.googleapis.com";
constexpr char kAudiance[] = "https://aistreams.googleapis.com/";
constexpr char kResourceNameFormat[] = "projects/-/serviceAccounts/%s";
constexpr char kGoogleApplicationCredentials[] =
    "GOOGLE_APPLICATION_CREDENTIALS";
constexpr char kClientEmailKey[] = "client_email";

}  // namespace

StatusOr<std::string> GetIdToken(const std::string& service_account) {
  auto channel =
      grpc::CreateChannel(kIAMGoogleAPI, grpc::GoogleDefaultCredentials());
  std::unique_ptr<IAMCredentials::Stub> stub(IAMCredentials::NewStub(channel));

  GenerateIdTokenRequest request;
  GenerateIdTokenResponse response;

  request.set_name(absl::StrFormat(kResourceNameFormat, service_account));
  request.set_audience(kAudiance);
  request.set_include_email(true);

  grpc::ClientContext ctx;
  auto status = stub->GenerateIdToken(&ctx, request, &response);
  if (!status.ok()) {
    LOG(ERROR) << status.error_message();
    return InternalError(
        "Encountered error while calling IAM service to generate ID token.");
  }
  return response.token();
}

StatusOr<std::string> GetIdTokenWithDefaultServiceAccount() {
  const char* cred_path = std::getenv(kGoogleApplicationCredentials);
  if (cred_path == nullptr) {
    return InternalError(
        "GOOGLE_APPLICATION_CREDENTIALS is not set. Please follow "
        "https://cloud.google.com/docs/authentication/getting-started to setup "
        "authentication.");
  }

  // Read json key file.
  std::string file_contents;
  auto status = file::GetContents(cred_path, &file_contents);
  if (!status.ok()) {
    LOG(ERROR) << status;
    return InvalidArgumentError(
        absl::StrFormat("Failed to get contents from file %s", cred_path));
  }

  rapidjson::Document doc;
  doc.Parse(file_contents);
  if (!doc.HasMember(kClientEmailKey)) {
    return InternalError("Failed to find client_email from the file.");
  }

  return GetIdToken(doc[kClientEmailKey].GetString());
}

}  // namespace aistreams
