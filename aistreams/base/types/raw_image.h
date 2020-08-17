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

#ifndef AISTREAMS_BASE_TYPES_RAW_IMAGE_H_
#define AISTREAMS_BASE_TYPES_RAW_IMAGE_H_

#include <string>

#include "aistreams/port/status.h"
#include "aistreams/proto/types/raw_image.pb.h"

namespace aistreams {

class RawImage {
 public:
  // Constructs a raw image of the specified height, width, and format.
  RawImage(int height, int width, RawImageFormat format);

  // Constructs a raw image from a RawImageDescriptor.
  explicit RawImage(const RawImageDescriptor&);

  // Constructs a raw image from a RawImageDescriptor and is
  // move initialized to the given bytes.
  RawImage(const RawImageDescriptor&, std::string&& bytes);

  // Constructs a zero height, zero width, SRGB image.
  RawImage();

  // Returns the height of the image.
  int height() const { return height_; }

  // Returns the width of the image.
  int width() const { return width_; }

  // Returns the number of channels of the image.
  int channels() const { return channels_; }

  // Returns the image format.
  RawImageFormat format() const { return raw_image_format_; }

  // Returns a reference to the i'th value of the image buffer.
  //
  // You must ensure i is in the range [0, size()).
  const uint8_t& operator()(size_t i) const {
    return reinterpret_cast<const uint8_t&>(data_[i]);
  }

  uint8_t& operator()(size_t i) {
    return const_cast<uint8_t&>(static_cast<const RawImage&>(*this)(i));
  }

  // Returns a pointer to the first value of the image.
  //
  // The valid values are in the contiguous address range
  // [data(), data()+size()).
  uint8_t* data() {
    return const_cast<uint8_t*>(static_cast<const RawImage&>(*this).data());
  }

  const uint8_t* data() const {
    return reinterpret_cast<const uint8_t*>(data_.data());
  }

  // Returns the total size of the image.
  size_t size() const { return data_.size(); }

  // Returns the released image buffer for the caller to acquire.
  std::string&& ReleaseBuffer() && { return std::move(data_); }

 private:
  int height_;
  int width_;
  int channels_;
  RawImageFormat raw_image_format_;
  std::string data_;
};

}  // namespace aistreams

#endif  // AISTREAMS_BASE_TYPES_RAW_IMAGE_H_
