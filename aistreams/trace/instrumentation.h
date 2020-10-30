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

#ifndef AISTREAMS_TRACE_INSTRUMENTATION_H_
#define AISTREAMS_TRACE_INSTRUMENTATION_H_

#include "absl/container/flat_hash_map.h"
#include "aistreams/proto/packet.pb.h"
#include "opencensus/trace/span.h"

namespace aistreams {
namespace trace {

// Instrument a packet header by setting its trace context with the given
// `probability`. The `probability` will be rounded to 4 decimal places.
void Instrument(PacketHeader* packet_header, double probability);

}  // namespace trace
}  // namespace aistreams

#endif  // AISTREAMS_TRACE_INSTRUMENTATION_H_
