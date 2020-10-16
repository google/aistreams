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

namespace aistreams {

Status RunPrinter() {
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

    // Print the packet.
    LOG(INFO) << p.DebugString();
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
