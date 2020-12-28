// Copyright 2020 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "aistreams/base/packet_receiver.h"

#include <utility>

#include "absl/time/clock.h"
#include "aistreams/port/canonical_errors.h"
#include "aistreams/port/status.h"
#include "aistreams/port/status_macros.h"
#include "aistreams/util/grpc_status_delegate.h"
#include "aistreams/util/random_string.h"

namespace aistreams {

namespace {
using ::aistreams::OffsetConfig;
using ::aistreams::util::MakeStatusFromRpcStatus;
using ::google::protobuf::Duration;

constexpr int kRandomConsumerNameLength = 8;

void RandomConsumerName(std::string* s) {
  *s = RandomString(kRandomConsumerNameLength);
}

OffsetConfig ToProtoOffsetConfig(
    const OffsetOptions::PositionType& offset_position) {
  OffsetConfig proto_offset_config;
  if (absl::holds_alternative<OffsetOptions::SpecialOffset>(offset_position)) {
    OffsetOptions::SpecialOffset special_offset =
        absl::get<OffsetOptions::SpecialOffset>(offset_position);
    if (special_offset == OffsetOptions::SpecialOffset::kOffsetBeginning) {
      proto_offset_config.set_special_offset(OffsetConfig::OFFSET_BEGINNING);
    } else if (special_offset == OffsetOptions::SpecialOffset::kOffsetEnd) {
      proto_offset_config.set_special_offset(OffsetConfig::OFFSET_END);
    }
  } else if (absl::holds_alternative<int64_t>(offset_position)) {
    proto_offset_config.set_seek_position(absl::get<int64_t>(offset_position));
  } else if (absl::holds_alternative<absl::Time>(offset_position)) {
    absl::Time seek_time = absl::get<absl::Time>(offset_position);
    proto_offset_config.mutable_seek_time()->set_seconds(
        absl::ToUnixSeconds(seek_time));
    proto_offset_config.mutable_seek_time()->set_nanos(absl::ToInt64Nanoseconds(
        seek_time -
        absl::FromUnixSeconds(proto_offset_config.seek_time().seconds())));
  }
  return proto_offset_config;
}

Duration ToProtoDuration(absl::Duration d) {
  Duration pd;
  pd.set_seconds(d / absl::Seconds(1));
  pd.set_nanos((d - absl::Seconds(pd.seconds())) / absl::Nanoseconds(1));
  return pd;
}

}  // namespace

PacketReceiver::PacketReceiver(const Options& options) : options_(options) {}

Status PacketReceiver::Initialize() {
  StreamChannel::Options stream_channel_options;
  stream_channel_options.connection_options = options_.connection_options;
  stream_channel_options.stream_name = options_.stream_name;
  // Set the timeout in connection options for unary-styled packet receiving.
  if (options_.receiver_mode == ReceiverMode::UnaryReceive &&
      options_.timeout > absl::ZeroDuration() &&
      options_.timeout < absl::InfiniteDuration()) {
    stream_channel_options.connection_options.rpc_options.timeout =
        std::min(stream_channel_options.connection_options.rpc_options.timeout,
                 options_.timeout);
  }
  auto stream_channel_status_or = StreamChannel::Create(stream_channel_options);
  if (!stream_channel_status_or.ok()) {
    LOG(ERROR) << stream_channel_status_or.status();
    return UnknownError("Failed to create a StreamChannel");
  }
  stream_channel_ = std::move(stream_channel_status_or).ValueOrDie();

  stub_ = std::move(StreamServer::NewStub(stream_channel_->GetChannel()));
  if (stub_ == nullptr) {
    return UnknownError("Failed to create a gRPC stub");
  }

  if (options_.receiver_mode == ReceiverMode::Auto ||
      options_.receiver_mode == ReceiverMode::Replay) {
    current_receiver_mode_ = ReceiverMode::Replay;
    AIS_RETURN_IF_ERROR(InitializeReplayStream());
  }

  if (options_.receiver_mode == ReceiverMode::Auto ||
      options_.receiver_mode == ReceiverMode::StreamingReceive) {
    current_receiver_mode_ = ReceiverMode::StreamingReceive;
    AIS_RETURN_IF_ERROR(InitializeReceivePacket());
  }

  return OkStatus();
}

Status PacketReceiver::InitializeReceivePacket() {
  ReceivePacketsRequest streaming_request;
  if (options_.receiver_name.empty()) {
    RandomConsumerName(streaming_request.mutable_consumer_name());
  } else {
    streaming_request.set_consumer_name(options_.receiver_name);
  }

  if (options_.offset_options.reset_offset) {
    *streaming_request.mutable_offset_config() =
        ToProtoOffsetConfig(options_.offset_options.offset_position);
  }

  if (options_.timeout > absl::ZeroDuration() &&
      options_.timeout < absl::InfiniteDuration()) {
    *streaming_request.mutable_timeout() = ToProtoDuration(options_.timeout);
  }

  auto ctx_status_or = stream_channel_->MakeClientContext();
  if (!ctx_status_or.ok()) {
    LOG(ERROR) << ctx_status_or.status();
    return InternalError("Failed to create a grpc client context");
  }
  ctx_[ReceiverMode::StreamingReceive] = std::move(ctx_status_or).ValueOrDie();
  streaming_readers_[ReceiverMode::StreamingReceive] = stub_->ReceivePackets(
      ctx_[ReceiverMode::StreamingReceive].get(), streaming_request);
  if (streaming_readers_[ReceiverMode::StreamingReceive] == nullptr) {
    return UnknownError("Failed to create a ClientReader for streaming RPC");
  }
  return OkStatus();
}

Status PacketReceiver::InitializeReplayStream() {
  ReplayStreamRequest replay_stream_request;
  if (options_.receiver_name.empty()) {
    RandomConsumerName(replay_stream_request.mutable_consumer_name());
  } else {
    replay_stream_request.set_consumer_name(options_.receiver_name);
  }

  if (options_.timeout > absl::ZeroDuration() &&
      options_.timeout < absl::InfiniteDuration()) {
    *replay_stream_request.mutable_timeout() =
        ToProtoDuration(options_.timeout);
  }

  if (options_.offset_options.reset_offset) {
    *replay_stream_request.mutable_offset_config() =
        ToProtoOffsetConfig(options_.offset_options.offset_position);
  }

  auto ctx_status_or = stream_channel_->MakeClientContext();
  if (!ctx_status_or.ok()) {
    LOG(ERROR) << ctx_status_or.status();
    return InternalError("Failed to create a grpc client context");
  }
  ctx_[ReceiverMode::Replay] = std::move(ctx_status_or).ValueOrDie();
  streaming_readers_[ReceiverMode::Replay] = stub_->ReplayStream(
      ctx_[ReceiverMode::Replay].get(), replay_stream_request);
  if (streaming_readers_[ReceiverMode::Replay] == nullptr) {
    return UnknownError("Failed to create a ClientReader for streaming RPC");
  }
  return OkStatus();
}

StatusOr<std::unique_ptr<PacketReceiver>> PacketReceiver::Create(
    const Options& options) {
  auto packet_receiver = std::make_unique<PacketReceiver>(options);
  AIS_RETURN_IF_ERROR(packet_receiver->Initialize());
  return packet_receiver;
}

Status PacketReceiver::Subscribe(const PacketCallback& callback) {
  Packet packet;
  while (true) {
    AIS_RETURN_IF_ERROR(Receive(&packet));
    Status s = callback(std::move(packet));
    if (!s.ok()) {
      if (IsCancelled(s)) {
        LOG(INFO) << "The subscriber has requested to cancel";
        break;
      } else {
        LOG(ERROR) << "PacketCallback returned non-ok status: " << s.message();
      }
    }

    if (options_.receiver_mode == ReceiverMode::UnaryReceive &&
        options_.unary_rpc_poll_interval > absl::ZeroDuration()) {
      absl::SleepFor(options_.unary_rpc_poll_interval);
    }
  }

  return OkStatus();
}

Status PacketReceiver::UnaryReceive(Packet* packet) {
  // Create a client context.
  auto ctx_status_or = stream_channel_->MakeClientContext();
  if (!ctx_status_or.ok()) {
    LOG(ERROR) << ctx_status_or.status();
    return InternalError("Failed to create a grpc client context");
  }
  auto ctx = std::move(ctx_status_or).ValueOrDie();

  // Make the unary rpc.
  ReceiveOnePacketRequest request;
  request.set_blocking(true);
  request.set_consumer_name(options_.receiver_name);
  // Apply the offset options only for the first unary request.
  if (unary_packets_received_ == 0 && options_.offset_options.reset_offset) {
    *request.mutable_offset_config() =
        ToProtoOffsetConfig(options_.offset_options.offset_position);
  }

  ReceiveOnePacketResponse response;
  grpc::Status grpc_status =
      stub_->ReceiveOnePacket(ctx.get(), request, &response);
  if (!grpc_status.ok()) {
    LOG(ERROR) << grpc_status.error_message();
    return MakeStatusFromRpcStatus(grpc_status);
  }
  ++unary_packets_received_;

  // Extract the response.
  if (!response.valid()) {
    return NotFoundError("The response does not contain a packet.");
  }
  *packet = std::move(*response.mutable_packet());
  return OkStatus();
}

Status PacketReceiver::StreamingReceive(Packet* packet) {
  bool first_receiving = first_receiving_;
  first_receiving_ = false;

  if (!streaming_readers_[current_receiver_mode_]->Read(packet)) {
    auto grpc_status = streaming_readers_[current_receiver_mode_]->Finish();
    if (first_receiving && options_.receiver_mode == ReceiverMode::Auto &&
        current_receiver_mode_ == ReceiverMode::StreamingReceive &&
        grpc_status.error_code() == grpc::StatusCode::OUT_OF_RANGE) {
      LOG(INFO) << "Switch to replay mode";
      current_receiver_mode_ = ReceiverMode::Replay;
      // Dispose unused client reader (ReceivePackets) while switching the
      // receiver mode.
      DisposeUnusedClientReader();
      return StreamingReceive(packet);
    } else {
      return MakeStatusFromRpcStatus(grpc_status);
    }
  }

  // Dispose unused client reader (could be either ReceivePackets or
  // ReplayStream).
  if (first_receiving && options_.receiver_mode == ReceiverMode::Auto) {
    DisposeUnusedClientReader();
  }

  return OkStatus();
}

Status PacketReceiver::Receive(Packet* packet) {
  if (options_.receiver_mode == ReceiverMode::UnaryReceive) {
    return UnaryReceive(packet);
  } else {
    return StreamingReceive(packet);
  }
}

void PacketReceiver::DisposeUnusedClientReader() {
  auto dispose = [this](ReceiverMode mode) {
    ctx_[mode]->TryCancel();
    streaming_readers_.erase(mode);
    ctx_.erase(mode);
  };

  if (current_receiver_mode_ == ReceiverMode::Replay) {
    dispose(ReceiverMode::StreamingReceive);
  }
  if (current_receiver_mode_ == ReceiverMode::StreamingReceive) {
    dispose(ReceiverMode::Replay);
  }
}

}  // namespace aistreams
