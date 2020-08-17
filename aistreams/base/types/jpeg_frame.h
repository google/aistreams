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

#ifndef AISTREAMS_BASE_TYPES_JPEG_FRAME_H_
#define AISTREAMS_BASE_TYPES_JPEG_FRAME_H_

#include <string>

#include "aistreams/port/status.h"

namespace aistreams {

// A JpegFrame holds a buffer of the jpeg encoded bytes.
// The elements in [data(), data()+size()) can be directly used for decoding.
class JpegFrame {
 public:
  // Construct/assign a jpeg frame from a string holding the encoded bytes.
  //
  // It is the user's responsibility to supply a valid jpeg byte string.
  explicit JpegFrame(std::string);

  // Access to the first byte of the encoded byte buffer.
  const char* data() const { return bytes_.data(); }

  // Size of the encoded byte buffer.
  size_t size() const { return bytes_.size(); }

  // Returns the released jpeg byte buffer for the caller to acquire.
  std::string&& ReleaseBuffer() && { return std::move(bytes_); }

  // Request default copy-control members.
  JpegFrame() = default;
  ~JpegFrame() = default;
  JpegFrame(const JpegFrame&) = default;
  JpegFrame(JpegFrame&&) = default;
  JpegFrame& operator=(const JpegFrame&) = default;
  JpegFrame& operator=(JpegFrame&&) = default;

 private:
  std::string bytes_;
};

}  // namespace aistreams

#endif  // AISTREAMS_BASE_TYPES_JPEG_FRAME_H_
