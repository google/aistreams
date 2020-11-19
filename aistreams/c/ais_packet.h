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

#ifndef AISTREAMS_C_AIS_PACKET_H_
#define AISTREAMS_C_AIS_PACKET_H_

#include <stddef.h>

#include "aistreams/c/ais_gstreamer_buffer.h"
#include "aistreams/c/ais_status.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct AIS_Packet AIS_Packet;

// --------------------------------------------------------------------------

// Returns a new default packet.
//
// Such a packet has undefined contents and is most useful to be used as the
// destination to receive a Packet of a known type.
//
// Returns a nullptr on failure.
extern AIS_Packet* AIS_NewPacket(AIS_Status* ais_status);

// Returns a new EOS packet holding the given reason.
//
// Returns a nullptr on failure.
extern AIS_Packet* AIS_NewEosPacket(const char* reason, AIS_Status* ais_status);

// Return a new packet of type AIS_PACKET_TYPE_STRING that holds the contents of
// the given null terminated string *without* including the null character.
//
// Returns a nullptr on failure.
extern AIS_Packet* AIS_NewStringPacket(const char* cstr,
                                       AIS_Status* ais_status);

// Return a new packet of type AIS_PACKET_TYPE_STRING that holds the bytes that
// are held between [src, src+count).
//
// Returns a nullptr on failure.
extern AIS_Packet* AIS_NewBytesPacket(const char* src, size_t count,
                                      AIS_Status* ais_status);

// Return a new packet of type AIS_PACKET_TYPE_GSTREAMER_BUFFER by moving the
// data from the given AIS_GstreamerBuffer.
//
// Usually, you should call the functions to set the caps string and have the
// data buffer assigned appropriately to the given AIS_GstreamerBuffer instance
// before calling this method.
//
// You still retain ownership of the given AIS_GstreamerBuffer and should
// remember to delete it. However, accessing its values after this call is
// undefined behavior.
//
// Returns a nullptr on failure.
extern AIS_Packet* AIS_NewGstreamerBufferPacket(
    AIS_GstreamerBuffer* ais_gstreamer_buffer, AIS_Status* ais_status);

// Delete a previously created status object.
extern void AIS_DeletePacket(AIS_Packet*);

// Returns whether the given ais_packet is an EOS indicator.
//
// If `reason` is not NULL and `ais_packet` is an EOS, then `reason` will
// indicate why the EOS was sent. The ownership of `reason` is transferred to
// the caller.
extern unsigned char AIS_IsEos(const AIS_Packet* ais_packet, char** reason);

// --------------------------------------------------------------------------
// You should generally not use the methods below unless you are defining new
// packet types or developing this library.

// Mark whether the given `ais_packet` is a key frame.
extern void AIS_SetIsKeyFrame(unsigned char is_key_frame,
                              AIS_Packet* ais_packet);

// Mark whether the given `ais_packet` is a frame head.
extern void AIS_SetIsFrameHead(unsigned char is_frame_head,
                               AIS_Packet* ais_packet);

#ifdef __cplusplus
} /* end extern "C" */
#endif

#endif  // AISTREAMS_C_AIS_PACKET_H_
