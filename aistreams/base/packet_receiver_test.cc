#include "aistreams/base/packet_receiver.h"

#include <thread>

#include "absl/time/clock.h"
#include "aistreams/mocks/mock_stream_service.h"
#include "aistreams/port/canonical_errors.h"
#include "aistreams/util/constants.h"
#include "google/protobuf/util/time_util.h"

namespace aistreams {

namespace {
using ::aistreams::constants::kStreamMetadataKeyName;
using ::aistreams::mocks::MockStreamService;
using ::google::protobuf::util::TimeUtil;
using ::testing::_;
using ::testing::Return;
using ::testing::Test;

constexpr char kConsumerName[] = "test-consumer";
constexpr char kStreamName[] = "test-stream";
constexpr char kStreamServerAddress[] = "localhost:6000";

constexpr absl::Duration kTimeout = absl::Seconds(10);
constexpr int kSeekOffset = 1234;
constexpr absl::Time kSeekTime = absl::FromUnixSeconds(1234);

class PacketReceiverTest : public Test {
 protected:
  void SetUp() override {
    stream_service_ = std::make_unique<MockStreamService>();
    grpc::ServerBuilder builder;
    builder.AddListeningPort(kStreamServerAddress,
                             grpc::InsecureServerCredentials());
    builder.RegisterService(stream_service_.get());
    builder.SetMaxReceiveMessageSize(-1);
    stream_server_ = builder.BuildAndStart();
    stream_server_worker_ = std::thread([this] { stream_server_->Wait(); });
  }

  void TearDown() override {
    stream_server_->Shutdown();
    stream_server_worker_.join();
  }

  static Packet MakePacket(int offset) {
    Packet packet;
    packet.mutable_header()->mutable_type()->set_type_id(PACKET_TYPE_STRING);
    packet.mutable_header()->mutable_timestamp()->set_seconds(offset);
    packet.mutable_header()->mutable_server_metadata()->set_offset(offset);
    packet.set_payload(std::to_string(offset));
    return packet;
  }

  static StatusOr<std::string> GetStreamName(
      const grpc::ServerContext *context) {
    auto it = context->client_metadata().find(kStreamMetadataKeyName);
    if (it == context->client_metadata().end()) {
      LOG(ERROR) << "Didn't get stream name.";
      return NotFoundError("Stream name not found.");
    }
    return std::string(it->second.data(), it->second.length());
  }

  std::unique_ptr<grpc::Server> stream_server_ = nullptr;
  std::unique_ptr<MockStreamService> stream_service_ = nullptr;
  std::thread stream_server_worker_;
};

TEST_F(PacketReceiverTest, UnaryReceive) {
  std::vector<Packet> packets = {MakePacket(0)};
  EXPECT_CALL(*stream_service_.get(), ReceiveOnePacket(_, _, _))
      .Times(2)
      .WillOnce([&](grpc::ServerContext *context,
                    const ReceiveOnePacketRequest *request,
                    ReceiveOnePacketResponse *response) {
        auto stream_name_status_or = GetStreamName(context);
        absl::Time deadline =
            absl::FromUnixNanos(context->deadline().time_since_epoch().count());
        EXPECT_LE(kTimeout - absl::Milliseconds(100), deadline - absl::Now());
        EXPECT_EQ(kStreamName, stream_name_status_or.ValueOrDie());
        EXPECT_EQ(kConsumerName, request->consumer_name());
        EXPECT_TRUE(request->has_offset_config());
        EXPECT_EQ(OffsetConfig::kSpecialOffset,
                  request->offset_config().config_case());
        EXPECT_EQ(OffsetConfig::OFFSET_BEGINNING,
                  request->offset_config().special_offset());
        response->set_valid(true);
        *response->mutable_packet() = packets[0];
        return ::grpc::Status();
      })
      .WillOnce(
          Return(::grpc::Status(::grpc::StatusCode::NOT_FOUND, "Not found")));

  PacketReceiver::Options options;
  options.receiver_name = kConsumerName;
  options.stream_name = kStreamName;
  options.offset_options.reset_offset = true;
  options.offset_options.offset_position =
      OffsetOptions::SpecialOffset::kOffsetBeginning;
  options.timeout = kTimeout;
  options.receiver_mode = ReceiverMode::UnaryReceive;
  options.connection_options.target_address = kStreamServerAddress;
  options.connection_options.ssl_options.use_insecure_channel = true;
  auto packet_receiver_status_or = PacketReceiver::Create(options);
  EXPECT_OK(packet_receiver_status_or);
  auto packet_receiver = std::move(packet_receiver_status_or).ValueOrDie();
  Packet packet;
  EXPECT_OK(packet_receiver->Receive(&packet));
  EXPECT_EQ(packets[0].ShortDebugString(), packet.ShortDebugString());
  EXPECT_EQ(StatusCode::kNotFound, packet_receiver->Receive(&packet).code());
}

TEST_F(PacketReceiverTest, UnaryReceiveWithoutOffset) {
  std::vector<Packet> packets = {MakePacket(0)};
  EXPECT_CALL(*stream_service_.get(), ReceiveOnePacket(_, _, _))
      .Times(2)
      .WillOnce([&](grpc::ServerContext *context,
                    const ReceiveOnePacketRequest *request,
                    ReceiveOnePacketResponse *response) {
        auto stream_name_status_or = GetStreamName(context);
        absl::Time deadline =
            absl::FromUnixNanos(context->deadline().time_since_epoch().count());
        EXPECT_LE(kTimeout - absl::Milliseconds(100), deadline - absl::Now());
        EXPECT_EQ(kStreamName, stream_name_status_or.ValueOrDie());
        EXPECT_EQ(kConsumerName, request->consumer_name());
        EXPECT_FALSE(request->has_offset_config());
        response->set_valid(true);
        *response->mutable_packet() = packets[0];
        return ::grpc::Status();
      })
      .WillOnce(
          Return(::grpc::Status(::grpc::StatusCode::NOT_FOUND, "Not found")));

  PacketReceiver::Options options;
  options.receiver_name = kConsumerName;
  options.stream_name = kStreamName;
  options.offset_options.reset_offset = false;
  options.timeout = kTimeout;
  options.receiver_mode = ReceiverMode::UnaryReceive;
  options.connection_options.target_address = kStreamServerAddress;
  options.connection_options.ssl_options.use_insecure_channel = true;
  auto packet_receiver_status_or = PacketReceiver::Create(options);
  EXPECT_OK(packet_receiver_status_or);
  auto packet_receiver = std::move(packet_receiver_status_or).ValueOrDie();
  Packet packet;
  EXPECT_OK(packet_receiver->Receive(&packet));
  EXPECT_EQ(packets[0].ShortDebugString(), packet.ShortDebugString());
  EXPECT_EQ(StatusCode::kNotFound, packet_receiver->Receive(&packet).code());
}

TEST_F(PacketReceiverTest, StreamingReceive) {
  std::vector<Packet> packets = {MakePacket(0)};
  EXPECT_CALL(*stream_service_.get(), ReceivePackets(_, _, _))
      .Times(1)
      .WillOnce([&](grpc::ServerContext *context,
                    const ReceivePacketsRequest *request,
                    grpc::ServerWriter<Packet> *stream) {
        auto stream_name_status_or = GetStreamName(context);
        absl::Time deadline =
            absl::FromUnixNanos(context->deadline().time_since_epoch().count());
        EXPECT_LE(absl::Hours(168), deadline - absl::Now());
        EXPECT_TRUE(request->has_timeout());
        EXPECT_EQ(kTimeout / absl::Nanoseconds(1),
                  TimeUtil::DurationToNanoseconds(request->timeout()));
        EXPECT_EQ(kStreamName, stream_name_status_or.ValueOrDie());
        EXPECT_EQ(kConsumerName, request->consumer_name());
        EXPECT_TRUE(request->has_offset_config());
        EXPECT_EQ(OffsetConfig::kSeekPosition,
                  request->offset_config().config_case());
        EXPECT_EQ(kSeekOffset, request->offset_config().seek_position());
        stream->Write(packets[0]);
        return ::grpc::Status(::grpc::StatusCode::INTERNAL, "Internal error");
      });

  PacketReceiver::Options options;
  options.receiver_name = kConsumerName;
  options.stream_name = kStreamName;
  options.offset_options.reset_offset = true;
  options.offset_options.offset_position = kSeekOffset;
  options.timeout = kTimeout;
  options.receiver_mode = ReceiverMode::StreamingReceive;
  options.connection_options.target_address = kStreamServerAddress;
  options.connection_options.ssl_options.use_insecure_channel = true;
  auto packet_receiver_status_or = PacketReceiver::Create(options);
  EXPECT_OK(packet_receiver_status_or);
  auto packet_receiver = std::move(packet_receiver_status_or).ValueOrDie();
  Packet packet;
  EXPECT_OK(packet_receiver->Receive(&packet));
  EXPECT_EQ(packets[0].ShortDebugString(), packet.ShortDebugString());
  EXPECT_EQ(StatusCode::kInternal, packet_receiver->Receive(&packet).code());
}

TEST_F(PacketReceiverTest, ReplayStream) {
  std::vector<Packet> packets = {MakePacket(0)};
  EXPECT_CALL(*stream_service_.get(), ReplayStream(_, _, _))
      .Times(1)
      .WillOnce([&](grpc::ServerContext *context,
                    const ReplayStreamRequest *request,
                    grpc::ServerWriter<Packet> *stream) {
        auto stream_name_status_or = GetStreamName(context);
        absl::Time deadline =
            absl::FromUnixNanos(context->deadline().time_since_epoch().count());
        EXPECT_LE(absl::Hours(168), deadline - absl::Now());
        EXPECT_TRUE(request->has_timeout());
        EXPECT_EQ(kTimeout / absl::Nanoseconds(1),
                  TimeUtil::DurationToNanoseconds(request->timeout()));
        EXPECT_EQ(kStreamName, stream_name_status_or.ValueOrDie());
        EXPECT_EQ(kConsumerName, request->consumer_name());
        EXPECT_TRUE(request->has_offset_config());
        EXPECT_EQ(OffsetConfig::kSeekTime,
                  request->offset_config().config_case());
        EXPECT_EQ(absl::ToUnixNanos(kSeekTime),
                  TimeUtil::TimestampToNanoseconds(
                      request->offset_config().seek_time()));
        stream->Write(packets[0]);
        return ::grpc::Status(::grpc::StatusCode::INTERNAL, "Internal error");
      });

  PacketReceiver::Options options;
  options.receiver_name = kConsumerName;
  options.stream_name = kStreamName;
  options.offset_options.reset_offset = true;
  options.offset_options.offset_position = kSeekTime;
  options.timeout = kTimeout;
  options.receiver_mode = ReceiverMode::Replay;
  options.connection_options.target_address = kStreamServerAddress;
  options.connection_options.ssl_options.use_insecure_channel = true;
  auto packet_receiver_status_or = PacketReceiver::Create(options);
  EXPECT_OK(packet_receiver_status_or);
  auto packet_receiver = std::move(packet_receiver_status_or).ValueOrDie();
  Packet packet;
  EXPECT_OK(packet_receiver->Receive(&packet));
  EXPECT_EQ(packets[0].ShortDebugString(), packet.ShortDebugString());
  EXPECT_EQ(StatusCode::kInternal, packet_receiver->Receive(&packet).code());
}

TEST_F(PacketReceiverTest, Subscribe) {
  std::vector<Packet> packets = {MakePacket(0), MakePacket(1), MakePacket(2)};
  EXPECT_CALL(*stream_service_.get(), ReceivePackets(_, _, _))
      .Times(1)
      .WillOnce([&](grpc::ServerContext *context,
                    const ReceivePacketsRequest *request,
                    grpc::ServerWriter<Packet> *stream) {
        for (const auto &packet : packets) {
          stream->Write(packet);
        }
        return ::grpc::Status(::grpc::StatusCode::NOT_FOUND,
                              "No packets found");
      });

  PacketReceiver::Options options;
  options.receiver_name = kConsumerName;
  options.stream_name = kStreamName;
  options.offset_options.reset_offset = true;
  options.offset_options.offset_position = kSeekOffset;
  options.timeout = kTimeout;
  options.receiver_mode = ReceiverMode::StreamingReceive;
  options.connection_options.target_address = kStreamServerAddress;
  options.connection_options.ssl_options.use_insecure_channel = true;

  auto packet_receiver_status_or = PacketReceiver::Create(options);
  EXPECT_OK(packet_receiver_status_or);
  auto packet_receiver = std::move(packet_receiver_status_or).ValueOrDie();
  int index = 0;
  EXPECT_OK(packet_receiver->Subscribe([&index, &packets](Packet packet) {
    if (index < static_cast<int>(packets.size()) - 1) {
      EXPECT_EQ(packets[index++].ShortDebugString(), packet.ShortDebugString());
      return OkStatus();
    }
    return CancelledError("Cancel error");
  }));
}

TEST_F(PacketReceiverTest, SubscribeWithErrorReceivingPacket) {
  std::vector<Packet> packets = {MakePacket(0), MakePacket(1), MakePacket(2)};
  EXPECT_CALL(*stream_service_.get(), ReceivePackets(_, _, _))
      .Times(1)
      .WillOnce([&](grpc::ServerContext *context,
                    const ReceivePacketsRequest *request,
                    grpc::ServerWriter<Packet> *stream) {
        for (const auto &packet : packets) {
          stream->Write(packet);
        }
        return ::grpc::Status(::grpc::StatusCode::NOT_FOUND,
                              "No packets found");
      });

  PacketReceiver::Options options;
  options.receiver_name = kConsumerName;
  options.stream_name = kStreamName;
  options.offset_options.reset_offset = true;
  options.offset_options.offset_position = kSeekOffset;
  options.timeout = kTimeout;
  options.receiver_mode = ReceiverMode::StreamingReceive;
  options.connection_options.target_address = kStreamServerAddress;
  options.connection_options.ssl_options.use_insecure_channel = true;

  auto packet_receiver_status_or = PacketReceiver::Create(options);
  EXPECT_OK(packet_receiver_status_or);
  auto packet_receiver = std::move(packet_receiver_status_or).ValueOrDie();
  int index = 0;
  EXPECT_EQ(StatusCode::kNotFound,
            packet_receiver
                ->Subscribe([&index, &packets](Packet packet) {
                  if (index < static_cast<int>(packets.size())) {
                    EXPECT_EQ(packets[index++].ShortDebugString(),
                              packet.ShortDebugString());
                  }
                  return OkStatus();
                })
                .code());
}

TEST_F(PacketReceiverTest, AutoPacketReceiverReceive) {
  std::vector<Packet> packets = {MakePacket(0), MakePacket(1), MakePacket(2)};
  EXPECT_CALL(*stream_service_.get(), ReceivePackets(_, _, _))
      .Times(1)
      .WillOnce([&](grpc::ServerContext *context,
                    const ReceivePacketsRequest *request,
                    grpc::ServerWriter<Packet> *stream) {
        return ::grpc::Status(::grpc::StatusCode::OUT_OF_RANGE,
                              "Offset out of range");
      });
  EXPECT_CALL(*stream_service_.get(), ReplayStream(_, _, _))
      .Times(1)
      .WillOnce([&](grpc::ServerContext *context,
                    const ReplayStreamRequest *request,
                    grpc::ServerWriter<Packet> *stream) {
        for (const auto &packet : packets) {
          stream->Write(packet);
        }
        return ::grpc::Status(::grpc::StatusCode::INTERNAL, "Internal Error");
      });

  PacketReceiver::Options options;
  options.receiver_name = kConsumerName;
  options.stream_name = kStreamName;
  options.offset_options.reset_offset = true;
  options.offset_options.offset_position = kSeekOffset;
  options.timeout = kTimeout;
  options.receiver_mode = ReceiverMode::Auto;
  options.connection_options.target_address = kStreamServerAddress;
  options.connection_options.ssl_options.use_insecure_channel = true;

  auto packet_receiver_status_or = PacketReceiver::Create(options);
  EXPECT_OK(packet_receiver_status_or);
  auto packet_receiver = std::move(packet_receiver_status_or).ValueOrDie();
  int index = 0;
  EXPECT_EQ(StatusCode::kInternal,
            packet_receiver
                ->Subscribe([&index, &packets](Packet packet) {
                  if (index < static_cast<int>(packets.size())) {
                    EXPECT_EQ(packets[index++].ShortDebugString(),
                              packet.ShortDebugString());
                  }
                  return OkStatus();
                })
                .code());
}

TEST_F(PacketReceiverTest, AutoPacketReceiverReceivesNoModeSwitch) {
  std::vector<Packet> packets = {MakePacket(0), MakePacket(1), MakePacket(2)};
  EXPECT_CALL(*stream_service_.get(), ReceivePackets(_, _, _))
      .Times(1)
      .WillOnce([&](grpc::ServerContext *context,
                    const ReceivePacketsRequest *request,
                    grpc::ServerWriter<Packet> *stream) {
        return ::grpc::Status(::grpc::StatusCode::INTERNAL, "Internal Error");
      });
  EXPECT_CALL(*stream_service_.get(), ReplayStream(_, _, _))
      .Times(1)
      .WillOnce([&](grpc::ServerContext *context,
                    const ReplayStreamRequest *request,
                    grpc::ServerWriter<Packet> *stream) {
        return ::grpc::Status(::grpc::StatusCode::NOT_FOUND, "Not found");
      });

  PacketReceiver::Options options;
  options.receiver_name = kConsumerName;
  options.stream_name = kStreamName;
  options.offset_options.reset_offset = true;
  options.offset_options.offset_position = kSeekOffset;
  options.timeout = kTimeout;
  options.receiver_mode = ReceiverMode::Auto;
  options.connection_options.target_address = kStreamServerAddress;
  options.connection_options.ssl_options.use_insecure_channel = true;

  auto packet_receiver_status_or = PacketReceiver::Create(options);
  EXPECT_OK(packet_receiver_status_or);
  auto packet_receiver = std::move(packet_receiver_status_or).ValueOrDie();
  int index = 0;
  EXPECT_EQ(
      StatusCode::kInternal,
      packet_receiver
          ->Subscribe([&index, &packets](Packet packet) { return OkStatus(); })
          .code());
}

}  // namespace

}  // namespace aistreams
