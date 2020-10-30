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

#ifndef AISTREAMS_C_C_API_H_
#define AISTREAMS_C_C_API_H_

#include <stddef.h>
#include <stdint.h>

#include "aistreams/c/ais_gstreamer_buffer.h"
#include "aistreams/c/ais_logging.h"
#include "aistreams/c/ais_packet.h"
#include "aistreams/c/ais_packet_as.h"
#include "aistreams/c/ais_status.h"

// --------------------------------------------------------------------------
// C API for AI Streamer.
//
// + We use the prefix AIS_ for everything in the API.
// + Objects are always passed around as pointers to opaque structs
//   and these structs are allocated/deallocated via the API.
// + Every call that has a AIS_Status* argument clears it on success
//   and fills it with error info on failure.
// + unsigned char is used for booleans, rather than using the "bool" macro
//   defined in stdbool.h.

#ifdef __cplusplus
extern "C" {
#endif

// --------------------------------------------------------------------------
// AIS_ConnectionOptions contain options to connect to an AI Streamer server.

typedef struct AIS_ConnectionOptions AIS_ConnectionOptions;

// Return a new connection options object.
extern AIS_ConnectionOptions* AIS_NewConnectionOptions(void);

// Delete a connection options object.
extern void AIS_DeleteConnectionOptions(AIS_ConnectionOptions*);

// Set the target address (ip:port) to the server (the k8s Ingress).
extern void AIS_SetTargetAddress(const char* target_adress,
                                 AIS_ConnectionOptions* options);

// Set to 1 when interacting with the google managed service; 0 otherwise.
extern void AIS_SetAuthenticateWithGoogle(
    unsigned char authenticate_with_google, AIS_ConnectionOptions* options);

// Set whether to use an insecure connection to the server.
extern void AIS_SetUseInsecureChannel(unsigned char use_insecure_channel,
                                      AIS_ConnectionOptions* options);

// Set the expected SSL domain name of the server.
//
// You are required to supply this if you do not use an insecure channel.
extern void AIS_SetSslDomainName(const char* ssl_domain_name,
                                 AIS_ConnectionOptions* options);

// Set the path to the root CA certificate.
//
// You are required to supply this if you do not use an insecure channel.
extern void AIS_SetSslRootCertPath(const char* ssl_root_cert_path,
                                   AIS_ConnectionOptions* options);

// --------------------------------------------------------------------------
// Functions to manage remote stream resources that reside on the server.

// Create a stream named `stream_name` on the server specified by `options`.
extern void AIS_CreateStream(const AIS_ConnectionOptions* options,
                             const char* stream_name, AIS_Status* ais_status);

// Delete a stream named `stream_name` on the server specified by `options`.
extern void AIS_DeleteStream(const AIS_ConnectionOptions* options,
                             const char* stream_name, AIS_Status* ais_status);

// --------------------------------------------------------------------------
// Functions to send packets to a stream on the server.

// Represents a packet sender.
typedef struct AIS_Sender AIS_Sender;

// Return a new packet sender object on success and NULL otherwise.
//
// On success, the AIS_Sender will have established a connection to the stream
// server specified in options and fully initialized for use.
extern AIS_Sender* AIS_NewSender(const AIS_ConnectionOptions* options,
                                 const char* stream_name,
                                 double trace_probability,
                                 AIS_Status* ais_status);

// Delete a packet sender object.
extern void AIS_DeleteSender(AIS_Sender* ais_sender);

// Send a packet through the packet sender.
//
// Typically, you would use one of the packet creaters in ais_packet.h to
// create a Packet. You are still the owner of the AIS_Packet and you should
// make sure to delete.
//
// After this call, the packet supplied will be in a deletable state; however,
// attemping to access values in the packet afterwards is undefined behavior.
extern void AIS_SendPacket(AIS_Sender* ais_sender, AIS_Packet* ais_packet,
                           AIS_Status* ais_status);

// --------------------------------------------------------------------------
// Functions to receive packets from a stream on the server.

// Represents a packet receiver.
typedef struct AIS_Receiver AIS_Receiver;

// Return a new packet receiver object. NULL otherwise.
//
// `stream_name`: specifies the name of the stream from which to receive from.
// `receiver_name`: specifies a name you use to receive. A random name will be
//                  generated for you if NULL.
extern AIS_Receiver* AIS_NewReceiver(const AIS_ConnectionOptions* options,
                                     const char* stream_name,
                                     const char* receiver_name,
                                     AIS_Status* ais_status);

// Delete a packet receiver object.
extern void AIS_DeleteReceiver(AIS_Receiver* ais_receiver);

// Receive a packet through the packet receiver.
//
// You need to create a new packet object to receive the value from the
// receiver. As usual, you will have to delete the packet when you are done.
//
// On success, the packet supplied will contain a fresh packet value from the
// receiver. On failure, the packet supplied will be in a deleteable state but
// accessing its value is undefined behavior. You may re-use the same packet to
// receive new values across multiple calls independent of whether the previous
// calls succeeded or failed.
//
// `timeout_in_sec`: If non-negative, then this call will block up to the
//                   the specified number of seconds to receive a packet
//                   from the server. If negative, then wait with no timeout.
extern void AIS_ReceivePacket(AIS_Receiver* ais_receiver,
                              AIS_Packet* ais_packet, int timeout_in_sec,
                              AIS_Status* ais_status);

#ifdef __cplusplus
}  // end extern "C"
#endif

#endif  // AISTREAMS_C_C_API_H_
