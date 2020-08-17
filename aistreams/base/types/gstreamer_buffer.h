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

#ifndef AISTREAMS_BASE_TYPES_GSTREAMER_BUFFER_H_
#define AISTREAMS_BASE_TYPES_GSTREAMER_BUFFER_H_

#include <string>

#include "absl/strings/string_view.h"
#include "aistreams/port/status.h"

namespace aistreams {

// A GstreamerBuffer contains the data in a GstBuffer and a string that
// describes its GstCaps (the "type").
class GstreamerBuffer {
 public:
  // Construct an empty GstreamerBuffer.
  GstreamerBuffer() = default;

  // Set the caps of the held data to caps_string.
  //
  // Usually, you would pass the result of the Gstreamer function
  // gst_caps_to_string into caps_string.
  void set_caps_string(absl::string_view caps_string) {
    caps_ = std::string(caps_string);
  }

  // Return the currently set caps string.
  const std::string& get_caps() const { return caps_; }

  // Return a pointer to the currently set caps as a cstring.
  //
  // The reference remains valid between calls to set_caps_string.
  const char* get_caps_cstr() const { return caps_.c_str(); }

  // Replaces the contents of the held data buffer by the bytes held between the
  // address range [src, src+size).
  //
  // Usually, you would use gst_buffer_map to obtain the starting address of the
  // GstBuffer and its size. You can then pass those into src and size.
  void assign(const char* src, size_t size) { bytes_.assign(src, src + size); }

  // Replaces the contents of the held data buffer by copying the argument.
  void assign(const std::string& s) { bytes_ = s; }

  // Replaces the contents of the held data buffer by moving the argument.
  void assign(std::string&& s) { bytes_ = std::move(s); }

  // Returns a pointer to the first value of the held data buffer.
  //
  // The reference remains valid between assigns.
  char* data() {
    return const_cast<char*>(static_cast<const GstreamerBuffer&>(*this).data());
  }

  const char* data() const { return bytes_.data(); }

  // Returns the size of held data buffer.
  size_t size() const { return bytes_.size(); }

  // Returns the released byte buffer for the caller to acquire.
  std::string&& ReleaseBuffer() && { return std::move(bytes_); }

  // Request default copy-control members.
  ~GstreamerBuffer() = default;
  GstreamerBuffer(const GstreamerBuffer&) = default;
  GstreamerBuffer(GstreamerBuffer&&) = default;
  GstreamerBuffer& operator=(const GstreamerBuffer&) = default;
  GstreamerBuffer& operator=(GstreamerBuffer&&) = default;

 private:
  std::string caps_;
  std::string bytes_;
};

}  // namespace aistreams

#endif  // AISTREAMS_BASE_TYPES_GSTREAMER_BUFFER_H_
