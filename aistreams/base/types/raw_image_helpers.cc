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

#include "aistreams/base/types/raw_image_helpers.h"

#include "absl/strings/str_format.h"
#include "aistreams/port/canonical_errors.h"
#include "aistreams/port/logging.h"

namespace aistreams {

int GetNumChannels(const RawImageFormat& format) {
  switch (format) {
    case RAW_IMAGE_FORMAT_SRGB:
      return 3;
    case RAW_IMAGE_FORMAT_UNKNOWN:
      LOG(WARNING) << "Received a raw image with an UNKNOWN format";
      return 1;
    default:
      LOG(ERROR) << absl::StrFormat(
          "The given raw image format (%d) is unimplemented", format);
      return 1;
  }
}

Status Validate(const RawImageDescriptor& desc) {
  if (desc.height() < 0) {
    return InvalidArgumentError(
        "Given a raw image descriptor of negative height");
  }
  if (desc.width() < 0) {
    return InvalidArgumentError(
        "Given a raw image descriptor of negative width");
  }
  return OkStatus();
}

StatusOr<int> GetBufferSize(const RawImageDescriptor& desc) {
  int height = desc.height();
  int width = desc.width();
  int channels = GetNumChannels(desc.format());
  if (height < 0 || width < 0 || channels < 0) {
    return InvalidArgumentError(absl::StrFormat(
        "The given raw image descriptor has negative dimensions/channels "
        "(height=%d, width=%d, channels=%d). They must be non-negative.",
        height, width, channels));
  }

  int pixels = 0;
  auto overflow = __builtin_mul_overflow(height, width, &pixels);
  if (overflow) {
    return InvalidArgumentError(absl::StrFormat(
        "Multiplication overflow when multiplying height (%d) and width (%d). "
        "Please contact us if you really need an image this large.",
        height, width));
  }

  int buf_size = 0;
  overflow = __builtin_mul_overflow(pixels, channels, &buf_size);
  if (overflow) {
    return InvalidArgumentError(
        absl::StrFormat("Multiplication overflow when multiply number of "
                        "pixels (%d) and the number of channels (%d), Please "
                        "contact us if you really need an image this large.",
                        pixels, channels));
  }

  return buf_size;
}

}  // namespace aistreams
