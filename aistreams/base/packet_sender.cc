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

#include "aistreams/base/packet_sender.h"

#include <utility>

#include "aistreams/port/canonical_errors.h"
#include "aistreams/port/status.h"
#include "aistreams/port/status_macros.h"
#include "aistreams/trace/instrumentation.h"

namespace aistreams {

PacketSender::PacketSender(const Options& options) : options_(options) {}

Status PacketSender::Initialize() {
  StreamChannel::Options stream_channel_options;
  stream_channel_options.connection_options = options_.connection_options;
  stream_channel_options.stream_name = options_.stream_name;
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

  if (!options_.enable_unary_rpc) {
    auto ctx_status_or = std::move(stream_channel_->MakeClientContext());
    if (!ctx_status_or.ok()) {
      LOG(ERROR) << ctx_status_or.status();
      return InternalError("Failed to create a grpc client context");
    }
    ctx_ = std::move(ctx_status_or).ValueOrDie();
    streaming_writer_ =
        std::move(stub_->SendPackets(ctx_.get(), &streaming_response_));
    if (streaming_writer_ == nullptr) {
      return UnknownError("Failed to create a ClientWriter for streaming RPC");
    }
  } else {
    LOG(INFO) << "Using unary rpc to send packets";
  }
  return OkStatus();
}

StatusOr<std::unique_ptr<PacketSender>> PacketSender::Create(
    const Options& options) {
  auto packet_sender = std::make_unique<PacketSender>(options);
  AIS_RETURN_IF_ERROR(packet_sender->Initialize());
  return packet_sender;
}

Status PacketSender::UnarySend(const Packet& packet) {
  SendOnePacketResponse response;
  auto ctx_status_or = std::move(stream_channel_->MakeClientContext());
  if (!ctx_status_or.ok()) {
    LOG(ERROR) << ctx_status_or.status();
    return InternalError("Failed to create a grpc client context");
  }
  ctx_ = std::move(ctx_status_or).ValueOrDie();
  grpc::Status grpc_status =
      stub_->SendOnePacket(ctx_.get(), packet, &response);
  if (!grpc_status.ok()) {
    LOG(ERROR) << grpc_status.error_message();
    return UnknownError("Encountered error calling RPC SendOnePacket");
  }
  if (!response.accepted()) {
    LOG(WARNING) << "The packet just sent was not accepted";
  }
  return OkStatus();
}

Status PacketSender::StreamingSend(const Packet& packet) {
  if (!streaming_writer_->Write(packet)) {
    return UnknownError("Failed to Write a packet into the RPC stream");
  }
  return OkStatus();
}

Status PacketSender::Send(const Packet& packet) {
  ::aistreams::trace::Instrument(const_cast<Packet&>(packet).mutable_header(),
                                 options_.trace_probability);
  if (streaming_writer_ == nullptr) {
    return UnarySend(packet);
  } else {
    return StreamingSend(packet);
  }
}

PacketSender::~PacketSender() {
  if (streaming_writer_ != nullptr) {
    if (!streaming_writer_->WritesDone()) {
      LOG(ERROR)
          << "Could not signal WritesDone() to gRPC server during cleanup";
    }
    grpc::Status grpc_status = streaming_writer_->Finish();
    if (!grpc_status.ok()) {
      LOG(ERROR) << "Could not Finish() the streaming writer during cleanup. "
                    "gRPC error code: "
                 << static_cast<int>(grpc_status.error_code())
                 << ", error message: " << grpc_status.error_message();
    }
  }
}

}  // namespace aistreams
