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

#ifndef AISTREAMS_BASE_PACKET_SENDER_H_
#define AISTREAMS_BASE_PACKET_SENDER_H_

#include "aistreams/base/connection_options.h"
#include "aistreams/base/stream_channel.h"
#include "aistreams/port/grpcpp.h"
#include "aistreams/port/status.h"
#include "aistreams/port/statusor.h"
#include "aistreams/proto/packet.pb.h"
#include "aistreams/proto/stream.grpc.pb.h"
#include "aistreams/proto/stream.pb.h"

namespace aistreams {

// Use this class to send a packet to a stream.
class PacketSender {
 public:
  // Options for configuring the packet sender.
  struct Options {
    // Options to configure the RPC connection.
    ConnectionOptions connection_options;

    // The stream name to connect to.
    //
    // Note: This is needed if `target_address` is to the ingress. You can leave
    // this empty if you are directly connecting to the stream server.
    std::string stream_name;

    // Set this true to use unary rpc to send packets.
    //
    // This is mainly useful for approximate profiling/packet tracing with
    // istio. It is not particularly efficient.
    bool enable_unary_rpc = false;

    // The probability to start a trace for each packet sent. If
    // `trace_probability` is set with a positive value, then packet sender
    // might update value for `trace_context` in the packet header.
    double trace_probability = 0;
  };

  // Creates and initializes an instance that is ready for use.
  static StatusOr<std::unique_ptr<PacketSender>> Create(const Options&);

  // Send the given packet.
  Status Send(const Packet&);

  // Use Create instead of the bare constructors.
  PacketSender(const Options&);
  ~PacketSender();

 private:
  Options options_;
  std::unique_ptr<StreamChannel> stream_channel_ = nullptr;
  std::unique_ptr<StreamServer::Stub> stub_ = nullptr;
  std::unique_ptr<grpc::ClientContext> ctx_ = nullptr;
  SendPacketsResponse streaming_response_;
  std::unique_ptr<grpc::ClientWriter<Packet>> streaming_writer_ = nullptr;

  Status Initialize();
  Status StreamingSend(const Packet&);
  Status UnarySend(const Packet&);
};

}  // namespace aistreams

#endif  // AISTREAMS_BASE_PACKET_SENDER_H_
