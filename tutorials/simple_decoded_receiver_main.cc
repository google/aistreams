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

// This example shows how you can programmatically receive RGB raw images from a
// stream. Note that the codec handling is taken care of behind the scenes, so
// you can use this directly with the stream ingested by, e.g.
// simple_ingester_main.cc.

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
ABSL_FLAG(int, queue_size, 300, "The size of the decoded image queue.");

namespace {
constexpr int kTryPopWaitSeconds = 1;
}  // namespace

namespace aistreams {

// Process the raw image packets that you receive.
//
// This simply prints image dimensions, so probably not the most useful thing to
// do. Nevertheless, it is the pattern here that you should pay attention to.
Status Work(std::unique_ptr<ReceiverQueue<Packet>> receiver_queue) {
  // The receiver queue will keep adding Packets of RawImage type as long as the
  // server stream has data. It is up to you to pull data off of it.
  //
  // If you don't pull the packets fast enough, then it will simply drop the
  // frames. If you pull faster than the stream can supply, then you have the
  // option to wait for some timeout.
  //
  // When the stream has reached the end, it will send you an EOS packet. You
  // should terminate or decide what to do otherwise when you see it.
  Packet p;
  std::string eos_reason;
  while (true) {
    // Timeout if no new packets are available. We just keep retrying, but you
    // can also quit/break.
    //
    // See aistreams/base/wrappers/receiver_queue.h for more info.
    if (!receiver_queue->TryPop(p, absl::Seconds(kTryPopWaitSeconds))) {
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
    // a RawImage. For this, you use PacketAs to adapt it to a RawImage.
    //
    // See aistreams/base/packet_as.h for more info on PacketAs.
    // See aistreams/base/types/raw_image.h for more info on RawImage.
    PacketAs<RawImage> packet_as(std::move(p));
    if (!packet_as.ok()) {
      LOG(ERROR) << packet_as.status();
      return UnknownError(
          "The server gave a non-RawImage Packet. Call upstream ingester "
          "and/or Google NOW!!");
    }
    RawImage raw_image = std::move(packet_as).ValueOrDie();

    // Print some stats to screen.
    LOG(INFO) << absl::StrFormat("h=%d w=%d c=%d", raw_image.height(),
                                 raw_image.width(), raw_image.channels());
  }
  return OkStatus();
}

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

  // Create a receiver queue that automatically gets packets from the server
  // stream and decodes it to Packets holding RawImages for you to directly use.
  //
  // See aistreams/cc/decoded_receivers.h for more details.
  int queue_size = absl::GetFlag(FLAGS_queue_size);
  int timeout_in_sec = absl::GetFlag(FLAGS_timeout_in_sec);
  auto receiver_queue = std::make_unique<ReceiverQueue<Packet>>();
  auto status = MakeDecodedReceiverQueue(receiver_options, queue_size,
                                         absl::Seconds(timeout_in_sec),
                                         receiver_queue.get());
  if (!status.ok()) {
    LOG(ERROR) << status;
    return UnknownError("Failed to create a queue of decoded images");
  }

  // Now that you have a raw image queue, you can do something with it.
  status = Work(std::move(receiver_queue));
  if (!status.ok()) {
    LOG(ERROR) << status;
    return UnknownError("Packet processing did not terminate normally");
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
