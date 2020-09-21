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

#include "aistreams/base/types/raw_image.h"

#include "absl/strings/str_format.h"
#include "aistreams/base/types/raw_image_helpers.h"
#include "aistreams/port/logging.h"
#include "aistreams/port/status.h"

namespace aistreams {

RawImage::RawImage() : RawImage(0, 0, RAW_IMAGE_FORMAT_SRGB) {}

RawImage::RawImage(const RawImageDescriptor &desc)
    : RawImage(desc.height(), desc.width(), desc.format()) {}

RawImage::RawImage(int height, int width, RawImageFormat format) {
  RawImageDescriptor desc;
  desc.set_height(height);
  desc.set_width(width);
  desc.set_format(format);
  auto status = Validate(desc);
  if (!status.ok()) {
    LOG(FATAL) << status;
  }

  height_ = desc.height();
  width_ = desc.width();
  raw_image_format_ = desc.format();
  channels_ = GetNumChannels(desc.format());

  auto image_buf_size_statusor = GetBufferSize(desc);
  if (!image_buf_size_statusor.ok()) {
    LOG(FATAL) << image_buf_size_statusor.status();
  }
  data_.resize(image_buf_size_statusor.ValueOrDie());
}

RawImage::RawImage(const RawImageDescriptor &desc, std::string &&bytes) {
  auto status = Validate(desc);
  if (!status.ok()) {
    LOG(FATAL) << status;
  }
  auto expected_bufsize_statusor = GetBufferSize(desc);
  if (!expected_bufsize_statusor.ok()) {
    LOG(FATAL) << expected_bufsize_statusor.status();
  }
  auto expected_bufsize = std::move(expected_bufsize_statusor).ValueOrDie();
  if (static_cast<size_t>(expected_bufsize) != bytes.size()) {
    LOG(FATAL) << absl::StrFormat(
        "Attempted to move construct a RawImage expecting %d bytes with a "
        "string containing %d bytes",
        expected_bufsize, bytes.size());
  }
  height_ = desc.height();
  width_ = desc.width();
  raw_image_format_ = desc.format();
  channels_ = GetNumChannels(desc.format());
  data_ = std::move(bytes);
}

}  // namespace aistreams
