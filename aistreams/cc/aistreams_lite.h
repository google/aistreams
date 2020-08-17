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

#ifndef AISTREAMS_CC_AISTREAMS_LITE_H_
#define AISTREAMS_CC_AISTREAMS_LITE_H_

#include "aistreams/base/connection_options.h"
#include "aistreams/base/management_client.h"
#include "aistreams/base/packet.h"
#include "aistreams/base/packet_receiver.h"
#include "aistreams/base/packet_sender.h"

// ---------------------------------------------------------------------
// C++ types that may be converted to/from a Packet.
// We call these the "elementary" types.
#include <string>

#include "aistreams/base/types/gstreamer_buffer.h"
#include "aistreams/base/types/jpeg_frame.h"
#include "aistreams/base/types/raw_image.h"
// All protobuf message types are also elementary.

#endif  // AISTREAMS_CC_AISTREAMS_LITE_H_
