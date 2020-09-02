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

#ifndef AISTREAMS_C_AIS_GSTREAMER_BUFFER_H_
#define AISTREAMS_C_AIS_GSTREAMER_BUFFER_H_

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// An AIS_GstreamerBuffer object represents a GstBuffer along with the GstCaps
// of the GstPad that it arrived from. One can think of it as a GstBuffer whose
// "type" is known.
typedef struct AIS_GstreamerBuffer AIS_GstreamerBuffer;

// Return a new gstreamer buffer object.
extern AIS_GstreamerBuffer* AIS_NewGstreamerBuffer(void);

// Delete a previously created gstreamer buffer object.
extern void AIS_DeleteGstreamerBuffer(AIS_GstreamerBuffer*);

// Set the caps c-string of the given AIS_GstreamerBuffer by copying what's
// presented in caps_cstr.
extern void AIS_GstreamerBufferSetCapsString(
    const char* caps_cstr, AIS_GstreamerBuffer* ais_gstreamer_buffer);

// Return an unowned c-string that describes the caps.
//
// This reference is valid until the next call to
// AIS_GstreamerBufferSetCapsString.
extern const char* AIS_GstreamerBufferGetCapsString(
    const AIS_GstreamerBuffer* ais_gstreamer_buffer);

// Set the data held in the given AIS_GstreamerBuffer to those given in the
// address range [src, src+count) by copying.
extern void AIS_GstreamerBufferAssign(
    const char* src, size_t count, AIS_GstreamerBuffer* ais_gstreamer_buffer);

// Returns the size (number of bytes) held in the given AIS_GstreamerBuffer.
extern size_t AIS_GstreamerBufferSize(
    const AIS_GstreamerBuffer* ais_gstreamer_buffer);

// Copies the data held in the given AIS_GstreamerBuffer to the address starting
// from dst.
//
// It is the caller's responsibility to ensure that dst is allocated with enough
// memory (at least AIS_GstreamerBufferSize).
extern void AIS_GstreamerBufferCopyTo(
    const AIS_GstreamerBuffer* ais_gstreamer_buffer, char* dst);

#ifdef __cplusplus
} /* end extern "C" */
#endif

#endif  // AISTREAMS_C_AIS_GSTREAMER_BUFFER_H_
