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

#include "aistreams/cc/decoded_receivers.h"

#include <functional>
#include <memory>
#include <thread>

#include "absl/strings/str_format.h"
#include "aistreams/base/packet_flags.h"
#include "aistreams/gstreamer/gstreamer_raw_image_yielder.h"
#include "aistreams/gstreamer/type_utils.h"
#include "aistreams/port/canonical_errors.h"
#include "aistreams/port/logging.h"
#include "aistreams/port/status.h"
#include "aistreams/port/status_macros.h"
#include "aistreams/port/statusor.h"

namespace aistreams {

namespace {

constexpr int kRetrySeconds = 1;

class ImageProducer {
 public:
  struct Options {
    absl::Duration timeout;
    std::unique_ptr<ReceiverQueue<Packet>> source_packet_queue;
    std::shared_ptr<ProducerConsumerQueue<Packet>> dest_image_packet_pcqueue;
  };

  static StatusOr<std::unique_ptr<ImageProducer>> Create(Options&& options) {
    auto image_producer = std::make_unique<ImageProducer>(std::move(options));
    auto status = image_producer->Initialize();
    if (!status.ok()) {
      return status;
    }
    return image_producer;
  }

  Status Initialize() {
    // Initialize the packet header queue.
    packet_header_pcqueue_ =
        std::move(std::make_unique<ProducerConsumerQueue<PacketHeader>>(
            source_packet_queue_->capacity()));

    // We pull the first packet from the source stream and determine whether it
    // has the correct Packet type to even be decodable.
    auto first_packet_statusor = PullSourcePacket();
    if (!first_packet_statusor.ok()) {
      LOG(ERROR) << first_packet_statusor.status();
      return UnavailableError("Unable to get the first packet from the server");
    }

    auto first_gstreamer_buffer_statusor =
        ToGstreamerBuffer(std::move(first_packet_statusor).ValueOrDie());
    if (!first_gstreamer_buffer_statusor.ok()) {
      LOG(ERROR) << first_gstreamer_buffer_statusor.status();
      return InvalidArgumentError(
          "Given a server stream that cannot be interpretted/decoded as a "
          "sequence of raw images");
    }
    auto first_gstreamer_buffer =
        std::move(first_gstreamer_buffer_statusor).ValueOrDie();

    // The packet stream type give by the caller is valid. Proceed to create a
    // GstreamerRawImageYielder to manage/run a raw image decoding pipeline.
    GstreamerRawImageYielder::Options yielder_options;
    yielder_options.caps_string = first_gstreamer_buffer.get_caps();
    yielder_options.callback =
        std::bind(&ImageProducer::PushImagePacket, this, std::placeholders::_1);
    auto yielder_statusor = GstreamerRawImageYielder::Create(yielder_options);
    if (!yielder_statusor.ok()) {
      LOG(ERROR) << yielder_statusor.status();
      return InternalError("Unable to create a GstreamerRawImageYielder");
    }
    yielder_ = std::move(yielder_statusor).ValueOrDie();

    // Remember to feed the first packet (othewise it will be dropped).
    auto status = yielder_->Feed(first_gstreamer_buffer);
    if (!status.ok()) {
      LOG(ERROR) << status;
      return InternalError(
          "Unable to successfully feed the first gstreamer buffer");
    }
    return OkStatus();
  }

  ImageProducer(Options&& options)
      : timeout_(options.timeout),
        source_packet_queue_(std::move(options.source_packet_queue)),
        dest_image_packet_pcqueue_(
            std::move(options.dest_image_packet_pcqueue)) {}

  // Helper to pull a single packet from the source packet stream.
  StatusOr<Packet> PullSourcePacket() {
    Packet p;
    if (!source_packet_queue_->TryPop(p, timeout_)) {
      return UnavailableError(
          absl::StrFormat("The server has not yielded any source packets "
                          "within the timeout (%s)",
                          absl::FormatDuration(timeout_)));
    }
    return p;
  }

  // Callback used to receive a decoded image from gstreamer.
  //
  // It will form a RawImage Packet from the decoded image and move it into the
  // output image receiver queue.
  Status PushImagePacket(StatusOr<RawImage> raw_image_statusor) {
    if (!raw_image_statusor.ok()) {
      // We will detect/push EOS packets separately in Work().
      if (IsResourceExhausted(raw_image_statusor.status())) {
        return OkStatus();
      } else {
        LOG(ERROR) << raw_image_statusor.status();
        return InternalError(
            "Got an unexpected error from the given StatusOr<RawImage>");
      }
    }

    // Form a RawImage Packet.
    auto packet_statusor =
        MakePacket(std::move(raw_image_statusor).ValueOrDie());
    if (!packet_statusor.ok()) {
      LOG(ERROR) << packet_statusor.status();
      return InternalError("Unable to create a raw image packet");
    }
    auto packet = std::move(packet_statusor).ValueOrDie();

    // Restore the corresponding frame head's header information.
    PacketHeader frame_head_header;
    if (packet_header_pcqueue_->TryPop(frame_head_header)) {
      *packet.mutable_header()->mutable_timestamp() =
          frame_head_header.timestamp();
      *packet.mutable_header()->mutable_addenda() = frame_head_header.addenda();
      *packet.mutable_header()->mutable_server_metadata() =
          frame_head_header.server_metadata();
      packet.mutable_header()->set_trace_context(
          frame_head_header.trace_context());
    }

    // Try to push a RawImage Packet onto the pcqueue.
    // Drop if it is already full.
    dest_image_packet_pcqueue_->TryEmplace(std::move(packet));
    return OkStatus();
  }

  // Helper to push an EOS Packet shared producer/consumer queue.
  Status PushEosPacket(const std::string& reason) {
    auto eos_packet_statusor = MakeEosPacket(reason);
    if (!eos_packet_statusor.ok()) {
      LOG(ERROR) << eos_packet_statusor.status();
      return InternalError("Couldn't create an EOS packet");
    }
    // Block until EOS can be delivered.
    dest_image_packet_pcqueue_->Emplace(
        std::move(eos_packet_statusor).ValueOrDie());
    return OkStatus();
  }

  // Helper to feed a convert/feed a Packet into the Gstreamer for decoding.
  //
  // Feed will queue headers for packets that are frame heads (i.e. the first
  // in a sequence of packets that form a single coded picture). It will then
  // push the payload into Gstreamer for decoding.
  //
  // PushImagePacket will then receive decoded frames and pop headers pushed by
  // Feed in FIFO order. Since Gstreamer outputs decoded frames in order, the
  // decoded frame recovers the corresponding frame head's header information.
  Status Feed(Packet packet) {
    // Queue packet headers for frame heads.
    if (IsPacketFlagsSet(PacketFlags::kIsFrameHead, packet)) {
      auto p = std::make_unique<PacketHeader>(packet.header());
      while (
          !packet_header_pcqueue_->TryPush(p, absl::Seconds(kRetrySeconds))) {
        LOG(WARNING) << "The header queue is full. The decoder is experiencing "
                        "high input load. ";
      }
    }

    auto gstreamer_buffer_statusor = ToGstreamerBuffer(std::move(packet));
    if (!gstreamer_buffer_statusor.ok()) {
      return gstreamer_buffer_statusor.status();
    }
    return yielder_->Feed(std::move(gstreamer_buffer_statusor).ValueOrDie());
  }

  // Main loop of the decoder thread.
  //
  // By the time this is run, the Gstreamer pipeline has already been
  // initialized and the first probe packet fed.
  Status Work() {
    std::string termination_message;

    while (dest_image_packet_pcqueue_.use_count() > 1) {
      // Get a Packet from the source stream.
      auto packet_statusor = PullSourcePacket();
      if (!packet_statusor.ok()) {
        termination_message = packet_statusor.status().error_message();
        break;
      }
      auto packet = std::move(packet_statusor).ValueOrDie();

      // Feed the source packet into the decoder if it is not EOS.
      // Break the loop otherwise.
      if (IsEos(packet)) {
        termination_message = "The raw image stream has ended";
        break;
      } else {
        auto status = Feed(std::move(packet));
        if (!status.ok()) {
          termination_message = packet_statusor.status().error_message();
          break;
        }
      }
    }

    // Shut the decoder down and push a final EOS packet to notify the
    // consumer.
    auto status = yielder_->SignalEOS();
    if (!status.ok()) {
      LOG(ERROR) << status;
    }
    return PushEosPacket(termination_message);
  }

 private:
  absl::Duration timeout_;
  std::unique_ptr<ReceiverQueue<Packet>> source_packet_queue_;
  std::shared_ptr<ProducerConsumerQueue<Packet>> dest_image_packet_pcqueue_;
  std::unique_ptr<ProducerConsumerQueue<PacketHeader>> packet_header_pcqueue_;
  std::unique_ptr<GstreamerRawImageYielder> yielder_;
};

}  // namespace

Status MakeDecodedReceiverQueue(
    const ReceiverOptions& options, int queue_size, absl::Duration timeout,
    ReceiverQueue<Packet>* dest_packet_receiver_queue) {
  // Create a receiver queue that gets source packets from the stream server.
  //
  // Ownership will be transferred into the decoder background thread below.
  auto src_packet_receiver_queue = std::make_unique<ReceiverQueue<Packet>>();
  auto status =
      MakePacketReceiverQueue(options, src_packet_receiver_queue.get());
  if (!status.ok()) {
    LOG(ERROR) << status;
    return UnknownError("Failed to create the source packet receiver queue");
  }

  // Create a producer/consumer queue for raw image packets.
  //
  // Two shares of ownership are created.
  // + The first share will be transferred into the decoder background thread,
  //   who is acting as the producer.
  // + The second share will be given to the caller, where they will consume
  //   the raw images for the application logic.
  auto packetized_image_pcqueue =
      std::make_shared<ProducerConsumerQueue<Packet>>(queue_size);
  *dest_packet_receiver_queue = ReceiverQueue<Packet>(packetized_image_pcqueue);

  // Create the ImageProducer.
  //
  // Note that this will pull the first packet from the stream server to learn
  // whether it is feasible to proceed.
  ImageProducer::Options image_producer_options;
  image_producer_options.timeout = timeout;
  image_producer_options.source_packet_queue =
      std::move(src_packet_receiver_queue);
  image_producer_options.dest_image_packet_pcqueue =
      std::move(packetized_image_pcqueue);
  auto image_producer_statusor =
      ImageProducer::Create(std::move(image_producer_options));
  if (!image_producer_statusor.ok()) {
    return image_producer_statusor.status();
  }
  auto image_producer = std::move(image_producer_statusor).ValueOrDie();

  // Launch the main loop of the image producer in a background thread.
  std::thread image_producer_worker(
      [image_producer = std::move(image_producer)]() {
        // Block to run the image producer.
        //
        // This terminates succesfully when any of the following is met:
        // + The source packet queue passes an EOS.
        // + The destination image packet queue is released by the caller.
        auto status = image_producer->Work();
        if (!status.ok()) {
          LOG(ERROR) << status;
        }
        return;
      });
  image_producer_worker.detach();

  return OkStatus();
}

}  // namespace aistreams
