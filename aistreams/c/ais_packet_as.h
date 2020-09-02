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

#ifndef AISTREAMS_C_AIS_PACKET_AS_H_
#define AISTREAMS_C_AIS_PACKET_AS_H_

#include "aistreams/c/ais_packet.h"
#include "aistreams/c/ais_status.h"

#ifdef __cplusplus
extern "C" {
#endif

// An AIS_PacketAs represents an object that adapts an AIS_Packet as a
// specific value type.
//
// Example Usage
// -------------
//
// ```
// // Adapt/interpret an AIS_Packet as an AIS_GstreamerBuffer.
// AIS_PacketAs* ais_packet_as =
//     AIS_NewGstreamerBufferPacketAs(ais_packet, ais_status);
//
// // Make sure the adaptation is successful.
// if (ais_packet_as == NULL) {
//   // report/return an error here (inspect ais_status to get more info).
// }
//
// // Now you can get a pointer to the held AIS_GstreamerBuffer.
// //
// // Remember that you should not delete this pointer directly; the resource is
// // cleaned up when you delete the AIS_PacketAs as shown later.
// const AIS_GstreamerBuffer *ais_gstreamer_buffer =
//   (const AIS_GstreamerBuffer*) AIS_PacketAsValue(ais_packet_as);
//
// // Do stuff with your value type...
//
// // Cleanup the PacketAs when you're done. ais_gstreamer_buffer is dangling
// // after this call so remember to not access its value.
// AIS_DeleteGstreamerBufferPacketAs(ais_packet_as);
// ```
typedef struct AIS_PacketAs AIS_PacketAs;

// Returns an (unowned) pointer to the value type object that the given
// AIS_PacketAs (non null) holds.
//
// The returned pointer is valid within the lifetime of the given AIS_PacketAs.
//
// You may cast the returned void* to a pointer of the known AIS value type.
const void* AIS_PacketAsValue(AIS_PacketAs* ais_packet_as);

// Returns a new AIS_PacketAs that holds an AIS_GstreamerBuffer value type.
// The returned object has a lifetime independent of the given AIS_Packet.
//
// Accessing the value of ais_packet after this call is undefined. However, you
// are still to clean it by calling AIS_DeletePacket.
//
// Returns NULL on failure.
extern AIS_PacketAs* AIS_NewGstreamerBufferPacketAs(AIS_Packet* ais_packet,
                                                    AIS_Status* ais_status);

// Delete an AIS_PacketAs holding an AIS_GstreamerBuffer value type.
extern void AIS_DeleteGstreamerBufferPacketAs(AIS_PacketAs*);

#ifdef __cplusplus
} /* end extern "C" */
#endif

#endif  // AISTREAMS_C_AIS_PACKET_AS_H_
