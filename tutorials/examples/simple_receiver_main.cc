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

// This example shows how you can programmatically receive packets of an
// arbitrary type. We receive the Greeting message sent by simple_sender_main.

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
#include "examples/hello.pb.h"

ABSL_FLAG(std::string, target_address, "",
          "Address (ip:port) to the data ingress.");
ABSL_FLAG(std::string, ssl_root_cert_path, "",
          "Path to the data ingress' ssl certificate.");
ABSL_FLAG(bool, authenticate_with_google, true,
          "Set to true if you are sending to a stream on the Google managed "
          "service; false otherwise.");
ABSL_FLAG(std::string, stream_name, "", "Name of the stream to receive from.");
ABSL_FLAG(int, timeout_in_sec, 5,
          "Seconds to wait for the server to deliver a new packet");

namespace aistreams {

Status RunReceiver() {
  // Options to configure the receiver.
  //
  // See aistreams/base/wrappers/receivers.h for more details.
  ReceiverOptions receiver_options;

  // Configure the connection to the data ingress.
  //
  // These have the usual conventions (see simple_ingester_main.cc for more
  // details.)
  ConnectionOptions connection_options;
  connection_options.target_address = absl::GetFlag(FLAGS_target_address);
  connection_options.ssl_options.ssl_root_cert_path =
      absl::GetFlag(FLAGS_ssl_root_cert_path);
  connection_options.authenticate_with_google =
      absl::GetFlag(FLAGS_authenticate_with_google);
  receiver_options.connection_options = connection_options;

  // Specify the stream to receive from.
  receiver_options.stream_name = absl::GetFlag(FLAGS_stream_name);

  // Create a receiver queue that automatically gets packets from the server.
  //
  // This receiver will stop pulling packets from the server if it fills up. No
  // packets will be dropped.
  //
  // See aistreams/base/wrappers/receivers.h for more info.
  auto receiver_queue = std::make_unique<ReceiverQueue<Packet>>();
  auto status = MakePacketReceiverQueue(receiver_options, receiver_queue.get());
  if (!status.ok()) {
    return UnknownError("Failed to create a packet receiver queue");
  }

  // Keep receiving packets until the sender says it is done.
  Packet p;
  std::string eos_reason;
  int timeout_in_sec = absl::GetFlag(FLAGS_timeout_in_sec);
  while (true) {
    // Timeout if no packets have arrived. We simply keep retrying here, but you
    // can quit/break.
    if (!receiver_queue->TryPop(p, absl::Seconds(timeout_in_sec))) {
      LOG(INFO) << "The receiver queue is currently empty";
      continue;
    }

    // When you do get a Packet, you should check if it is an EOS.
    //
    // See aistreams/base/util/packet_utils.h for more info.
    if (IsEos(p, &eos_reason)) {
      LOG(INFO) << "Got EOS with reason: \"" << eos_reason << "\"";
      break;
    }

    // Now you are sure it is a data packet, but you need to be sure that it is
    // a Greeting. For this, you use PacketAs to adapt it to a Greeting.
    //
    // See aistreams/base/packet_as.h for more info on PacketAs.
    // See examples/hello.proto for the Greeting message.
    PacketAs<examples::Greeting> packet_as(std::move(p));
    if (!packet_as.ok()) {
      LOG(ERROR) << packet_as.status();
      return UnknownError(
          "The server gave a non-Greeting Packet. Call upstream ingester "
          "and/or Google NOW!!");
    }
    examples::Greeting greeting = std::move(packet_as).ValueOrDie();

    // Print the contents of the greeting.
    LOG(INFO) << greeting.DebugString();
  }

  return OkStatus();
}

}  // namespace aistreams

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  absl::ParseCommandLine(argc, argv);
  auto status = aistreams::RunReceiver();
  if (!status.ok()) {
    LOG(ERROR) << status;
    return EXIT_FAILURE;
  }
  return 0;
}
