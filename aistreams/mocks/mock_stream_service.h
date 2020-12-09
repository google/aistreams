#ifndef AISTREAMS_BASE_MOCK_STREAM_SERVICE_H_
#define AISTREAMS_BASE_MOCK_STREAM_SERVICE_H_

#include "aistreams/port/gmock.h"
#include "aistreams/proto/stream.grpc.pb.h"

namespace aistreams {
namespace mocks {

class MockStreamService : public StreamServer::Service {
 public:
  MockStreamService() = default;
  ~MockStreamService() override = default;

  MOCK_METHOD(grpc::Status, SendPackets,
              (grpc::ServerContext * context,
               grpc::ServerReader<Packet> *stream,
               SendPacketsResponse *response),
              (override));

  MOCK_METHOD(grpc::Status, SendOnePacket,
              (grpc::ServerContext * context, const Packet *packet,
               SendOnePacketResponse *response),
              (override));

  MOCK_METHOD(grpc::Status, ReceivePackets,
              (grpc::ServerContext * context,
               const ReceivePacketsRequest *request,
               grpc::ServerWriter<Packet> *stream),
              (override));

  MOCK_METHOD(grpc::Status, ReceiveOnePacket,
              (grpc::ServerContext * context,
               const ReceiveOnePacketRequest *request,
               ReceiveOnePacketResponse *response),
              (override));

  MOCK_METHOD(grpc::Status, ReplayStream,
              (grpc::ServerContext * context,
               const ReplayStreamRequest *request,
               grpc::ServerWriter<Packet> *stream),
              (override));

  // MockStreamService is neither copyable nor movable.
  MockStreamService(const MockStreamService &) = delete;
  MockStreamService &operator=(const MockStreamService &) = delete;
};

}  // namespace mocks
}  // namespace aistreams

#endif  // AISTREAMS_BASE_MOCK_STREAM_SERVICE_H_
