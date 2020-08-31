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

#include "aistreams/base/util/raw_image_utils.h"

#include <algorithm>
#include <sstream>
#include <string>

#include "absl/strings/str_format.h"
#include "aistreams/port/canonical_errors.h"
#include "aistreams/port/logging.h"
#include "aistreams/proto/types/raw_image.pb.h"
#include "aistreams/util/file_helpers.h"

namespace aistreams {

Status ToPpmFile(absl::string_view file_name, const RawImage& raw_image) {
  std::string file_contents(absl::StrFormat(
      "P6\n%d %d\n255\n", raw_image.width(), raw_image.height()));
  auto data_start = reinterpret_cast<const char*>(raw_image.data());
  std::copy(data_start, data_start + raw_image.size(),
            std::back_inserter(file_contents));
  return file::SetContents(file_name, file_contents);
}

Status FromPpmFile(absl::string_view file_name, RawImage* dst) {
  std::string file_contents;
  auto status = file::GetContents(file_name, &file_contents);
  if (!status.ok()) {
    LOG(ERROR) << status;
    return InvalidArgumentError(
        absl::StrFormat("Failed to get contents from file %s", file_name));
  }

  std::string ppm_version;
  int width, height;
  int max_color_value;
  std::istringstream is(file_contents);
  is >> ppm_version >> width >> height >> max_color_value;
  if (!is) {
    return InvalidArgumentError(
        absl::StrFormat("Failed to read the PPM file header from %s (is it "
                        "written with ToPpmFile?)",
                        file_name));
  }
  std::string data(is.str().substr(
      is.tellg() + static_cast<std::istringstream::pos_type>(1)));

  RawImageDescriptor desc;
  desc.set_width(width);
  desc.set_height(height);
  desc.set_format(RAW_IMAGE_FORMAT_SRGB);
  LOG(INFO) << data.size();
  *dst = RawImage(desc, std::move(data));
  return OkStatus();
}

}  // namespace aistreams
