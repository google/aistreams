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

#ifndef AISTREAMS_GSTREAMER_AIS_TYPE_UTILS_H_
#define AISTREAMS_GSTREAMER_AIS_TYPE_UTILS_H_

#include <stddef.h>

#include "aistreams/c/c_api.h"

#ifdef __cplusplus
extern "C" {
#endif

// Return a new AIS_GstreamerBuffer by moving in the contents of the given
// AIS_Packet. Returns NULL on failure.
//
// This function will fail when the given AIS_Packet is not compatible with
// being converted to an AIS_GstreamerBuffer. In this case, the AIS_Status will
// tell the reason.
//
// Regardless of success/failure, you retain ownership of the given AIS_Packet
// and you should remember to delete it. However, accessing its values after a
// call to this function is undefined behavior.
extern AIS_GstreamerBuffer* AIS_ToGstreamerBuffer(AIS_Packet* ais_packet,
                                                  AIS_Status* ais_status);

#ifdef __cplusplus
} /* end extern "C" */
#endif

#endif  // AISTREAMS_GSTREAMER_AIS_TYPE_UTILS_H_
