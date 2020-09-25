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

// This example shows how you can programmatically send an arbitrary Packets
// stream. It assumes that you already have a stream created in the server; if
// you don't know how to do this, please refer to the README.md (e.g. using
// aisctl).

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
ABSL_FLAG(std::string, stream_name, "",
          "Name of the stream to ingest videos into.");

ABSL_FLAG(int, greeting_iterations, 10,
          "Number of greeting iterations to send.");
ABSL_FLAG(std::string, greeting_message, "Hello!",
          "The greeting message to send.");
ABSL_FLAG(int, milliseconds_between_greetings, 1000,
          "Number of milliseconds to wait before sending the next greeting.");

namespace aistreams {

Status RunSender() {
  // Options to configure the sender.
  //
  // See aistreams/base/wrappers/sender.h for more details.
  SenderOptions sender_options;

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
  sender_options.connection_options = connection_options;

  // Specify the stream to send into.
  sender_options.stream_name = absl::GetFlag(FLAGS_stream_name);

  // Create a sender.
  //
  // See aistreams/base/wrappers/senders.h for more info.
  std::unique_ptr<PacketSender> sender;
  auto status = MakePacketSender(sender_options, &sender);
  if (!status.ok()) {
    LOG(ERROR) << status;
    return UnknownError("Failed to create a packet sender");
  }

  // Send data packets. In this case, we keep sending the Greeting
  // message, just populated differently.
  //
  // Obviously, this is perhaps not all that useful in real life. But the
  // pattern here is what's important. Actually, you can send any protobuf
  // message whatsoever.
  int greeting_iterations = absl::GetFlag(FLAGS_greeting_iterations);
  std::string greeting_message = absl::GetFlag(FLAGS_greeting_message);
  int milliseconds_between_greetings =
      absl::GetFlag(FLAGS_milliseconds_between_greetings);

  for (int i = 0; i < greeting_iterations; ++i) {
    // Populate the greeting message.
    examples::Greeting greeting;
    greeting.set_iterations(i + 1);
    greeting.set_greeting(greeting_message);

    // Create a protobuf typed Packet. Notice that you do not need to do
    // anything special for the Packet to be typed correctly. Just give it your
    // data (move it in for efficiency if it is huge; overkill for this case).
    //
    // See aistreams/base/make_packet.h for more info.
    auto packet_statusor = MakePacket(std::move(greeting));
    if (!packet_statusor.ok()) {
      LOG(ERROR) << packet_statusor.status();
      return UnknownError("Couldn't make a packet. Call Google NOW!!");
    }
    auto packet = std::move(packet_statusor).ValueOrDie();

    // Send the Packet.
    auto status = sender->Send(packet);
    LOG(INFO) << "Sent " << i+1 << "'th packet.";
    if (!status.ok()) {
      LOG(ERROR) << status;
      return UnknownError("Failed to send a packet.");
    }

    // Not needed, but easier on the eyes for testing.
    absl::SleepFor(absl::Milliseconds(milliseconds_between_greetings));
  }

  // Send an EOS packet to indicate you're done.
  //
  // See aistreams/base/util/packet_utils.h for more info.
  auto eos_packet_statusor = MakeEosPacket("Successfully sent all messages");
  status = sender->Send(std::move(eos_packet_statusor).ValueOrDie());
  if (!status.ok()) {
    LOG(ERROR) << status;
    LOG(ERROR) << "Failed to mark the end of the stream with EOS";
  }

  LOG(INFO) << "Done!";

  return OkStatus();
}

}  // namespace aistreams

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  absl::ParseCommandLine(argc, argv);
  auto status = aistreams::RunSender();
  if (!status.ok()) {
    LOG(ERROR) << status;
    return EXIT_FAILURE;
  }
  return 0;
}
