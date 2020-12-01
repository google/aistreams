#include "aistreams/util/grpc_status_delegate.h"

#include "aistreams/port/gtest.h"

namespace aistreams {
namespace util {

namespace {
using ::aistreams::Status;
using ::aistreams::StatusCode;

TEST(MakeStatusFromRpcError, AllCodes) {
  struct {
    grpc::StatusCode grpc;
    StatusCode expected;
  } expected_codes[]{
      {grpc::StatusCode::OK, StatusCode::kOk},
      {grpc::StatusCode::CANCELLED, StatusCode::kCancelled},
      {grpc::StatusCode::UNKNOWN, StatusCode::kUnknown},
      {grpc::StatusCode::INVALID_ARGUMENT, StatusCode::kInvalidArgument},
      {grpc::StatusCode::DEADLINE_EXCEEDED, StatusCode::kDeadlineExceeded},
      {grpc::StatusCode::NOT_FOUND, StatusCode::kNotFound},
      {grpc::StatusCode::ALREADY_EXISTS, StatusCode::kAlreadyExists},
      {grpc::StatusCode::PERMISSION_DENIED, StatusCode::kPermissionDenied},
      {grpc::StatusCode::UNAUTHENTICATED, StatusCode::kUnauthenticated},
      {grpc::StatusCode::RESOURCE_EXHAUSTED, StatusCode::kResourceExhausted},
      {grpc::StatusCode::FAILED_PRECONDITION, StatusCode::kFailedPrecondition},
      {grpc::StatusCode::ABORTED, StatusCode::kAborted},
      {grpc::StatusCode::OUT_OF_RANGE, StatusCode::kOutOfRange},
      {grpc::StatusCode::UNIMPLEMENTED, StatusCode::kUnimplemented},
      {grpc::StatusCode::INTERNAL, StatusCode::kInternal},
      {grpc::StatusCode::UNAVAILABLE, StatusCode::kUnavailable},
      {grpc::StatusCode::DATA_LOSS, StatusCode::kDataLoss},
  };

  for (auto const& codes : expected_codes) {
    std::string const message = "test message";
    auto const original = grpc::Status(codes.grpc, message);
    auto const expected = Status(codes.expected, message);
    auto const actual1 = MakeStatusFromRpcStatus(original);
    auto const actual2 = MakeStatusFromRpcStatus(codes.grpc, message);
    EXPECT_EQ(expected, actual1);
    EXPECT_EQ(expected, actual2);
  }
}
}  // namespace

}  // namespace util
}  // namespace aistreams
