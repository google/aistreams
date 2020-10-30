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

#include "absl/strings/str_format.h"
#include "aistreams/port/logging.h"
#include "opencensus/trace/propagation/trace_context.h"

namespace aistreams {
namespace trace {

namespace {
constexpr int kProbabilityScaleMultiplier = 10000;

absl::flat_hash_map<std::string, ::std::string> global_attrs;
absl::flat_hash_map<int,
                    std::shared_ptr<::opencensus::trace::ProbabilitySampler>>
    samplers;

std::shared_ptr<::opencensus::trace::ProbabilitySampler> GetProbabilitySampler(
    double probability) {
  int scaled_probability =
      static_cast<int>(probability * kProbabilityScaleMultiplier);
  auto it = samplers.find(scaled_probability);
  if (it == samplers.end()) {
    samplers.emplace(
        scaled_probability,
        std::make_shared<::opencensus::trace::ProbabilitySampler>(probability));
  }
  return samplers.find(scaled_probability)->second;
}
}  // namespace

void Instrument(::aistreams::PacketHeader* packet_header, double probability) {
  if (probability < 0 || probability > 1) {
    LOG(ERROR) << "invalid probability: " << probability;
    return;
  }

  if (packet_header == nullptr) {
    LOG(ERROR) << "packet header is nullptr";
    return;
  }

  const std::shared_ptr<::opencensus::trace::ProbabilitySampler> sampler =
      GetProbabilitySampler(probability);
  // The span generated below is not used to report to zipkin server. It is for
  // the generation of trace context. So here we put an empty string as the
  // placeholder for span name.
  const ::opencensus::trace::Span span =
      ::opencensus::trace::Span::StartSpan("", nullptr, sampler.get());
  packet_header->set_trace_context(
      ::opencensus::trace::propagation::ToTraceParentHeader(span.context()));
}

}  // namespace trace
}  // namespace aistreams
