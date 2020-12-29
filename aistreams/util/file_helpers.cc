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

#include "aistreams/util/file_helpers.h"

#include <dirent.h>
#include <stdio.h>
#include <sys/stat.h>

#include <cerrno>

#include "absl/strings/str_format.h"
#include "aistreams/port/canonical_errors.h"
#include "aistreams/util/file_path.h"

namespace aistreams {
namespace file {
Status GetContents(absl::string_view file_name, std::string* output) {
  FILE* fp = fopen(file_name.data(), "r");
  if (fp == NULL) {
    return InvalidArgumentError(
        absl::StrFormat("Can't find file: %s", file_name));
  }

  output->clear();
  while (!feof(fp)) {
    char buf[4096];
    size_t ret = fread(buf, 1, 4096, fp);
    if (ret == 0 && ferror(fp)) {
      return InternalError(
          absl::StrFormat("Error while reading file: %s", file_name));
    }
    output->append(std::string(buf, ret));
  }
  fclose(fp);
  return OkStatus();
}

Status SetContents(absl::string_view file_name, absl::string_view content) {
  FILE* fp = fopen(file_name.data(), "w");
  if (fp == NULL) {
    return InvalidArgumentError(
        absl::StrFormat("Can't open file: %s", file_name));
  }

  fwrite(content.data(), sizeof(char), content.size(), fp);
  size_t ret = fclose(fp);
  if (ret == 0 && ferror(fp)) {
    return InternalError(
        absl::StrFormat("Error while writing file: %s", file_name));
  }
  return OkStatus();
}

Status Exists(absl::string_view file_name) {
  struct stat buffer;
  int status;
  status = stat(file_name.data(), &buffer);
  if (status == 0) {
    return OkStatus();
  }
  switch (errno) {
    case EACCES:
      return PermissionDeniedError("Insufficient permissions.");
    default:
      return NotFoundError("The path does not exist.");
  }
}

}  // namespace file
}  // namespace aistreams
