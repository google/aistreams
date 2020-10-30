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

#include "aistreams/trace/instrumentation.h"

#include "aistreams/port/gtest.h"
#include "aistreams/proto/packet.pb.h"
#include "opencensus/trace/propagation/trace_context.h"

namespace aistreams {
namespace trace {

namespace {
using ::aistreams::PacketHeader;
using ::opencensus::trace::Span;
using ::opencensus::trace::SpanContext;
using ::opencensus::trace::propagation::FromTraceParentHeader;

TEST(Trace, Instrument) {
  PacketHeader packet_header;
  EXPECT_TRUE(packet_header.trace_context().empty());
  Instrument(&packet_header, 2);
  EXPECT_TRUE(packet_header.trace_context().empty());
  Instrument(&packet_header, -1);
  EXPECT_TRUE(packet_header.trace_context().empty());
  Instrument(&packet_header, 1);
  EXPECT_FALSE(packet_header.trace_context().empty());
  SpanContext span_context =
      FromTraceParentHeader(packet_header.trace_context());
  EXPECT_TRUE(span_context.trace_options().IsSampled());
  Instrument(&packet_header, 0);
  span_context = FromTraceParentHeader(packet_header.trace_context());
  EXPECT_FALSE(span_context.trace_options().IsSampled());
}

}  // namespace

}  // namespace trace
}  // namespace aistreams
