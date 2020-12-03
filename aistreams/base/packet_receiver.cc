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

#include "absl/random/random.h"
#include "absl/time/clock.h"
#include "aistreams/port/canonical_errors.h"
#include "aistreams/port/status.h"
#include "aistreams/port/status_macros.h"
#include "aistreams/util/grpc_status_delegate.h"

namespace aistreams {

namespace {
using ::aistreams::OffsetConfig;
using ::aistreams::util::MakeStatusFromRpcStatus;

constexpr int kRandomConsumerNameLength = 8;
constexpr char kRandomConsumerChars[] =
    "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

void RandomConsumerName(std::string* s) {
  thread_local static absl::BitGen bitgen;
  s->resize(kRandomConsumerNameLength);
  for (int i = 0; i < kRandomConsumerNameLength; ++i) {
    size_t rand_i = absl::Uniform(bitgen, 0u, sizeof(kRandomConsumerChars) - 2);
    (*s)[i] = kRandomConsumerChars[rand_i];
  }
  return;
}

OffsetConfig ToProtoOffsetConfig(
    const OffsetOptions::PositionType& offset_position) {
  OffsetConfig proto_offset_config;
  if (absl::holds_alternative<OffsetOptions::SpecialOffset>(offset_position)) {
    OffsetOptions::SpecialOffset special_offset =
        absl::get<OffsetOptions::SpecialOffset>(offset_position);
    if (special_offset == OffsetOptions::SpecialOffset::kOffsetBeginning) {
      proto_offset_config.set_special_offset(
          OffsetConfig_SpecialOffset_OFFSET_BEGINNING);
    } else if (special_offset == OffsetOptions::SpecialOffset::kOffsetEnd) {
      proto_offset_config.set_special_offset(
          OffsetConfig_SpecialOffset_OFFSET_END);
    }
  } else if (absl::holds_alternative<int64_t>(offset_position)) {
    proto_offset_config.set_position(absl::get<int64_t>(offset_position));
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
}  // namespace

PacketReceiver::PacketReceiver(const Options& options) : options_(options) {}

Status PacketReceiver::Initialize() {
  StreamChannel::Options stream_channel_options;
  stream_channel_options.connection_options = options_.connection_options;
  stream_channel_options.stream_name = options_.stream_name;
  // Set the timeout in connection options for unary-styled packet receiving.
  if (options_.enable_unary_rpc && options_.timeout > absl::ZeroDuration() &&
      options_.timeout < absl::InfiniteDuration()) {
    stream_channel_options.connection_options.rpc_options.timeout =
        options_.timeout;
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

  ReceivePacketsRequest streaming_request;
  if (options_.receiver_name.empty()) {
    std::string random_receiver_name;
    RandomConsumerName(&random_receiver_name);
    streaming_request.set_consumer_name(random_receiver_name);
  } else {
    streaming_request.set_consumer_name(options_.receiver_name);
  }

  if (options_.offset_options.reset_offset) {
    *streaming_request.mutable_offset_config() =
        ToProtoOffsetConfig(options_.offset_options.offset_position);
  }

  if (options_.timeout > absl::ZeroDuration() &&
      options_.timeout < absl::InfiniteDuration()) {
    absl::Duration timeout = options_.timeout;
    streaming_request.mutable_timeout()->set_seconds(
        absl::IDivDuration(timeout, absl::Seconds(1), &timeout));
    streaming_request.mutable_timeout()->set_nanos(
        absl::IDivDuration(timeout, absl::Nanoseconds(1), &timeout));
  }

  if (!options_.enable_unary_rpc) {
    auto ctx_status_or = std::move(stream_channel_->MakeClientContext());
    if (!ctx_status_or.ok()) {
      LOG(ERROR) << ctx_status_or.status();
      return InternalError("Failed to create a grpc client context");
    }
    ctx_ = std::move(ctx_status_or).ValueOrDie();
    streaming_reader_ =
        std::move(stub_->ReceivePackets(ctx_.get(), streaming_request));
    if (streaming_reader_ == nullptr) {
      return UnknownError("Failed to create a ClientReader for streaming RPC");
    }
  } else {
    LOG(INFO) << "Using unary rpc to receive packets";
  }
  return OkStatus();
}

StatusOr<std::unique_ptr<PacketReceiver>> PacketReceiver::Create(
    const Options& options) {
  auto packet_receiver = std::make_unique<PacketReceiver>(options);
  AIS_RETURN_IF_ERROR(packet_receiver->Initialize());
  return packet_receiver;
}

Status PacketReceiver::UnarySubscribe(const PacketCallback& callback) {
  while (true) {
    Packet packet;
    Status rpc_status = UnaryReceive(&packet);
    if (!rpc_status.ok()) {
      LOG(ERROR) << "Unary rpc returned non-ok status: "
                 << rpc_status.message();
    } else {
      Status s = callback(std::move(packet));
      if (!s.ok()) {
        if (IsCancelled(s)) {
          LOG(INFO) << "The subscriber has requested to cancel";
          break;
        } else {
          LOG(ERROR) << "PacketCallback returned non-ok status: "
                     << s.message();
        }
      }
    }

    if (options_.unary_rpc_poll_interval > absl::ZeroDuration()) {
      absl::SleepFor(options_.unary_rpc_poll_interval);
    }
  }
  return OkStatus();
}

Status PacketReceiver::StreamingSubscribe(const PacketCallback& callback) {
  Packet packet;
  while (streaming_reader_->Read(&packet)) {
    Status s = callback(std::move(packet));
    if (!s.ok()) {
      if (IsCancelled(s)) {
        LOG(INFO) << "The subscriber has requested to cancel";
        break;
      } else {
        LOG(ERROR) << "PacketCallback returned non-ok status: " << s.message();
      }
    }
  }
  return OkStatus();
}

Status PacketReceiver::Subscribe(const PacketCallback& callback) {
  if (streaming_reader_ == nullptr) {
    return UnarySubscribe(callback);
  } else {
    return StreamingSubscribe(callback);
  }
}

Status PacketReceiver::UnaryReceive(Packet* packet) {
  // Create a client context.
  auto ctx_status_or = std::move(stream_channel_->MakeClientContext());
  if (!ctx_status_or.ok()) {
    LOG(ERROR) << ctx_status_or.status();
    return InternalError("Failed to create a grpc client context");
  }
  ctx_ = std::move(ctx_status_or).ValueOrDie();

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
      stub_->ReceiveOnePacket(ctx_.get(), request, &response);
  if (!grpc_status.ok()) {
    LOG(ERROR) << grpc_status.error_message();
    return UnknownError("Encountered error calling ReceiveOnePacket RPC");
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
  if (!streaming_reader_->Read(packet)) {
    return MakeStatusFromRpcStatus(streaming_reader_->Finish());
  }
  return OkStatus();
}

Status PacketReceiver::Receive(Packet* packet) {
  if (streaming_reader_ == nullptr) {
    return UnaryReceive(packet);
  } else {
    return StreamingReceive(packet);
  }
}

}  // namespace aistreams
