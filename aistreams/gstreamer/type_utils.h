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

#ifndef AISTREAMS_GSTREAMER_TYPE_UTILS_H_
#define AISTREAMS_GSTREAMER_TYPE_UTILS_H_

#include "aistreams/base/packet.h"
#include "aistreams/base/types/gstreamer_buffer.h"
#include "aistreams/base/types/raw_image.h"
#include "aistreams/port/status.h"
#include "aistreams/port/statusor.h"

namespace aistreams {

// Converts a GstreamerBuffer into a RawImage.
// You should pass an rvalue for `gstreamer_buffer` if possible.
//
// Of course, this may not always be possible based just on the fact
// that a GstreamerBuffer can contain any caps whatsoever.
// In any case, the returned status will indicate why a conversion failed.
StatusOr<RawImage> ToRawImage(GstreamerBuffer gstreamer_buffer);

// Convert the given Packet into a GstreamerBuffer.
// You should pass an rvalue for `packet` if possible.
//
// The conversion may not always be possible; we only define this for packets
// for which this makes sense (e.g. Jpegs, RawImages, etc.) The status will
// indicate the reason if the conversion fails.
StatusOr<GstreamerBuffer> ToGstreamerBuffer(Packet packet);

// Convert the given RawImage into a GstreamerBuffer.
//
// You should pass an rvalue for `raw_image` if possible.
StatusOr<GstreamerBuffer> ToGstreamerBuffer(RawImage raw_image);

}  // namespace aistreams

#endif  // AISTREAMS_GSTREAMER_TYPE_UTILS_H_
