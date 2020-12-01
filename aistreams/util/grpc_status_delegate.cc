#include "aistreams/util/grpc_status_delegate.h"

namespace aistreams {
namespace util {

namespace {
using ::aistreams::StatusCode;

StatusCode MapStatusCode(grpc::StatusCode const& code) {
  switch (code) {
    case grpc::StatusCode::OK:
      return StatusCode::kOk;
    case grpc::StatusCode::CANCELLED:
      return StatusCode::kCancelled;
    case grpc::StatusCode::UNKNOWN:
      return StatusCode::kUnknown;
    case grpc::StatusCode::INVALID_ARGUMENT:
      return StatusCode::kInvalidArgument;
    case grpc::StatusCode::DEADLINE_EXCEEDED:
      return StatusCode::kDeadlineExceeded;
    case grpc::StatusCode::NOT_FOUND:
      return StatusCode::kNotFound;
    case grpc::StatusCode::ALREADY_EXISTS:
      return StatusCode::kAlreadyExists;
    case grpc::StatusCode::PERMISSION_DENIED:
      return StatusCode::kPermissionDenied;
    case grpc::StatusCode::UNAUTHENTICATED:
      return StatusCode::kUnauthenticated;
    case grpc::StatusCode::RESOURCE_EXHAUSTED:
      return StatusCode::kResourceExhausted;
    case grpc::StatusCode::FAILED_PRECONDITION:
      return StatusCode::kFailedPrecondition;
    case grpc::StatusCode::ABORTED:
      return StatusCode::kAborted;
    case grpc::StatusCode::OUT_OF_RANGE:
      return StatusCode::kOutOfRange;
    case grpc::StatusCode::UNIMPLEMENTED:
      return StatusCode::kUnimplemented;
    case grpc::StatusCode::INTERNAL:
      return StatusCode::kInternal;
    case grpc::StatusCode::UNAVAILABLE:
      return StatusCode::kUnavailable;
    case grpc::StatusCode::DATA_LOSS:
      return StatusCode::kDataLoss;
    default:
      return StatusCode::kUnknown;
  }
}

}  // namespace

aistreams::Status MakeStatusFromRpcStatus(grpc::Status const& status) {
  return MakeStatusFromRpcStatus(status.error_code(), status.error_message());
}

aistreams::Status MakeStatusFromRpcStatus(grpc::StatusCode code,
                                          absl::string_view message) {
  return aistreams::Status(MapStatusCode(code), message);
}

}  // namespace util
}  // namespace aistreams
