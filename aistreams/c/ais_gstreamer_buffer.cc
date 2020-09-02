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

#include "aistreams/c/ais_gstreamer_buffer.h"

#include <algorithm>

#include "aistreams/c/ais_gstreamer_buffer_internal.h"
#include "aistreams/c/ais_status_internal.h"
#include "aistreams/port/status.h"

using ::aistreams::GstreamerBuffer;
using ::aistreams::OkStatus;
using ::aistreams::Status;
using ::aistreams::StatusCode;

AIS_GstreamerBuffer* AIS_NewGstreamerBuffer() {
  return new AIS_GstreamerBuffer;
}

void AIS_DeleteGstreamerBuffer(AIS_GstreamerBuffer* ais_gstreamer_buffer) {
  delete ais_gstreamer_buffer;
}

void AIS_GstreamerBufferSetCapsString(
    const char* caps_cstr, AIS_GstreamerBuffer* ais_gstreamer_buffer) {
  ais_gstreamer_buffer->gstreamer_buffer.set_caps_string(caps_cstr);
}

const char* AIS_GstreamerBufferGetCapsString(
    const AIS_GstreamerBuffer* ais_gstreamer_buffer) {
  return ais_gstreamer_buffer->gstreamer_buffer.get_caps_cstr();
}

void AIS_GstreamerBufferAssign(const char* src, size_t count,
                               AIS_GstreamerBuffer* ais_gstreamer_buffer) {
  return ais_gstreamer_buffer->gstreamer_buffer.assign(src, count);
}

size_t AIS_GstreamerBufferSize(
    const AIS_GstreamerBuffer* ais_gstreamer_buffer) {
  return ais_gstreamer_buffer->gstreamer_buffer.size();
}

void AIS_GstreamerBufferCopyTo(const AIS_GstreamerBuffer* ais_gstreamer_buffer,
                               char* dst) {
  std::copy(ais_gstreamer_buffer->gstreamer_buffer.data(),
            ais_gstreamer_buffer->gstreamer_buffer.data() +
                ais_gstreamer_buffer->gstreamer_buffer.size(),
            dst);
  return;
}
