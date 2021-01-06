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

#ifndef AISTREAMS_BASE_PACKET_RECEIVER_H_
#define AISTREAMS_BASE_PACKET_RECEIVER_H_

#include "absl/time/time.h"
#include "aistreams/base/connection_options.h"
#include "aistreams/base/offset_options.h"
#include "aistreams/base/stream_channel.h"
#include "aistreams/port/grpcpp.h"
#include "aistreams/port/status.h"
#include "aistreams/port/statusor.h"
#include "aistreams/proto/packet.pb.h"
#include "aistreams/proto/stream.grpc.pb.h"
#include "aistreams/proto/stream.pb.h"

namespace aistreams {

enum class ReceiverMode {
  StreamingReceive,
  UnaryReceive,
  Replay,
  // Receiver will switch to replay mode if receiver returns OUT_OF_RANGE from
  // streaming receive mode.
  Auto,
};

// The callback type used to subscribe to incoming packets.
//
// Return a kCancelled Status to shut down.
using PacketCallback = std::function<Status(Packet)>;

// Use this class to subscribe to a stream for packets.
class PacketReceiver {
 public:
  // Options for configuring the packet receiver.
  struct Options {
    // Options to configure the RPC connection.
    ConnectionOptions connection_options;

    // Options to specify the offset to start receiving.
    OffsetOptions offset_options;

    // The stream name to connect to.
    //
    // Note: This is needed if `target_address` is to the ingress. You can leave
    // this empty if you are directly connecting to the stream server.
    std::string stream_name;

    // The receiver name for the stream server to identify each receiver.
    std::string receiver_name;

    // The interval between unary rpc polls.
    absl::Duration unary_rpc_poll_interval = absl::ZeroDuration();

    // The timeout to receive a packet. This configuration is primarily used for
    // the `Receive` method; if there isn't an available packet for a duration
    // longer than the `timeout`, then the `Receive` method call will be
    // terminated.
    absl::Duration timeout = absl::InfiniteDuration();

    // The receiver mode. Default receiver mode is the streaming receiver.
    ReceiverMode receiver_mode = ReceiverMode::StreamingReceive;
  };

  // Creates and initializes an instance that is ready for use.
  static StatusOr<std::unique_ptr<PacketReceiver>> Create(const Options&);

  // Receive a packet.
  //
  // This call blocks until a packet is available or when an error occurs.
  //
  // Note: You should use exactly one of Receive or Subscribe. In the case that
  // you need both, run them in two distinct PacketReceivers.
  Status Receive(Packet*);

  // Start an incoming stream of packets.
  //
  // The given callback is called for each newly arrived packet.
  //
  // Note: You should use exactly one of Receive or Subscribe. In the case that
  // you need both, run them in two distinct PacketReceivers.
  Status Subscribe(const PacketCallback&);

  // Use Create instead of the bare constructors.
  PacketReceiver(const Options&);
  ~PacketReceiver() = default;

 private:
  Options options_;
  ReceiverMode current_receiver_mode_;
  std::unique_ptr<StreamChannel> stream_channel_ = nullptr;
  std::unique_ptr<StreamServer::Stub> stub_ = nullptr;
  std::unordered_map<ReceiverMode, std::unique_ptr<grpc::ClientContext>> ctx_;
  std::unordered_map<ReceiverMode, std::unique_ptr<grpc::ClientReader<Packet>>>
      streaming_readers_;
  // Number of packets that have been received via the unary endpoint.
  int unary_packets_received_ = 0;
  bool first_receiving_ = true;

  Status Initialize();
  Status InitializeReceivePacket();
  Status InitializeReplayStream();
  Status StreamingReceive(Packet*);
  Status UnaryReceive(Packet*);
  void DisposeUnusedClientReader();
};

}  // namespace aistreams

#endif  // AISTREAMS_BASE_PACKET_RECEIVER_H_
