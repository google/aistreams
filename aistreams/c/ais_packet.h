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

#ifdef __cplusplus
} /* end extern "C" */
#endif

#endif  // AISTREAMS_C_AIS_PACKET_H_
