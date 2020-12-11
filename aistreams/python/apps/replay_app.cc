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

#include <memory>
#include <string>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/strings/str_format.h"
#include "absl/time/time.h"
#include "aistreams/cc/aistreams.h"
#include "aistreams/port/logging.h"
#include "aistreams/port/status.h"
#include "aistreams/port/statusor.h"

ABSL_FLAG(std::string, target_address, "localhost:50051",
          "Address (ip:port) to the AI Streams instance.");
ABSL_FLAG(std::string, stream_name, "", "Name of the stream to send to.");
ABSL_FLAG(bool, authenticate_with_google, false,
          "Set to true for the managed service; otherwise false.");
ABSL_FLAG(bool, use_insecure_channel, false, "Use an insecure channel.");
ABSL_FLAG(std::string, ssl_domain_name, "aistreams.googleapis.com",
          "The expected ssl domain name of the service.");
ABSL_FLAG(std::string, ssl_root_cert_path, "",
          "The path to the ssl root certificate.");
ABSL_FLAG(int, timeout_in_sec, 60,
          "The timeout for the server to deliver a packet. "
          "Active if non-negative.");
ABSL_FLAG(int, position_type, -1,
          "The position type. 0 for speciall offset, offset_beginning; 1 for "
          "special offset, offset_end; 2 for offset position; 3 for timestamp");
ABSL_FLAG(int64_t, start_offset, -1, "The start offset to seek to.");
ABSL_FLAG(int64_t, end_offset, -1, "The end offset to seek to.");
ABSL_FLAG(absl::Time, start_timestamp, absl::InfinitePast(),
          "The start timestamp to seek to.");
ABSL_FLAG(absl::Time, end_timestamp, absl::InfinitePast(),
          "The end timestamp to seek to.");
ABSL_FLAG(int64_t, num_packets, 0,
          "The number of received packets. It's used when position_type is set "
          "to 0, 1. The receiver will receive num_packets packets.");

namespace aistreams {

Status RunPrinter() {
  int64_t received_packets = 0;
  std::function<bool(const Packet&)> stop_receive = [](const Packet&) {
    return false;
  };
  OffsetOptions offset_option;
  int position_type = absl::GetFlag(FLAGS_position_type);
  if (position_type == 0 || position_type == 1) {
    offset_option.reset_offset = true;
    offset_option.offset_position =
        position_type == 0 ? OffsetOptions::SpecialOffset::kOffsetBeginning
                           : OffsetOptions::SpecialOffset::kOffsetEnd;
    int64_t num_packets = absl::GetFlag(FLAGS_num_packets);
    if (num_packets <= 0) {
      return InvalidArgumentError("num_packets should be > 0");
    }

    stop_receive = [&](const Packet&) {
      return received_packets >= num_packets;
    };
  } else if (position_type == 2) {
    int64_t start_offset = absl::GetFlag(FLAGS_start_offset),
            end_offset = absl::GetFlag(FLAGS_end_offset);
    if (start_offset < 0 || end_offset < 0 || start_offset > end_offset) {
      return InvalidArgumentError("start_offset and end_offset are invalid");
    }
    offset_option.offset_position = start_offset;
    stop_receive = [&](const Packet& p) {
      return p.header().server_metadata().offset() >= end_offset;
    };
  } else if (position_type == 3) {
    absl::Time start_timestamp = absl::GetFlag(FLAGS_start_timestamp),
               end_timestamp = absl::GetFlag(FLAGS_end_timestamp);
    if (start_timestamp == absl::InfinitePast() ||
        end_timestamp == absl::InfinitePast() ||
        start_timestamp > end_timestamp) {
      return InvalidArgumentError(
          "start_timestamp and end_timestamp are invalid");
    }
    offset_option.offset_position = start_timestamp;
    stop_receive = [&](const Packet& p) {
      ::google::protobuf::Timestamp ts = p.header().timestamp();
      return absl::FromUnixNanos(ts.seconds() * 1000000000 + ts.nanos()) >=
             end_timestamp;
    };
  }

  // Configure the receiver.
  ReceiverOptions receiver_options;
  ConnectionOptions connection_options;
  connection_options.target_address = absl::GetFlag(FLAGS_target_address);
  connection_options.ssl_options.use_insecure_channel =
      absl::GetFlag(FLAGS_use_insecure_channel);
  connection_options.ssl_options.ssl_root_cert_path =
      absl::GetFlag(FLAGS_ssl_root_cert_path);
  connection_options.ssl_options.ssl_domain_name =
      absl::GetFlag(FLAGS_ssl_domain_name);
  connection_options.authenticate_with_google =
      absl::GetFlag(FLAGS_authenticate_with_google);
  receiver_options.connection_options = connection_options;
  receiver_options.stream_name = absl::GetFlag(FLAGS_stream_name);
  receiver_options.receiver_mode = ReceiverMode::Replay;
  receiver_options.offset_options = offset_option;

  // Create a receiver queue to the stream.
  auto receiver_queue = std::make_unique<ReceiverQueue<Packet>>();
  auto status = MakePacketReceiverQueue(receiver_options, receiver_queue.get());
  if (!status.ok()) {
    return UnknownError("Failed to create a packet receiver queue");
  }
  int timeout_in_sec = absl::GetFlag(FLAGS_timeout_in_sec);
  absl::Duration timeout_duration = absl::InfiniteDuration();
  if (timeout_in_sec >= 0) {
    timeout_duration = absl::Seconds(timeout_in_sec);
  }

  // Keep receiving packets until EOS is reachced.
  Packet p;
  std::string eos_reason;
  while (true) {
    // Timeout if no packets have arrived.
    if (!receiver_queue->TryPop(p, timeout_duration)) {
      return DeadlineExceededError(absl::StrFormat(
          "No messages have been received in the last %d seconds.",
          timeout_in_sec));
    }

    // Check for EOS.
    if (IsEos(p, &eos_reason)) {
      LOG(INFO) << "Got EOS with reason: \"" << eos_reason << "\"";
      break;
    }

    received_packets++;
    // TODO: convert the stream of the packet to file.
    if (stop_receive(p)) {
      LOG(INFO) << "Complete receiving packets";
      break;
    }
  }

  return OkStatus();
}

}  // namespace aistreams

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  absl::ParseCommandLine(argc, argv);
  auto status = aistreams::RunPrinter();
  if (!status.ok()) {
    LOG(ERROR) << status;
  }
  return 0;
}
