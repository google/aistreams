#ifndef AISTREAMS_UTIL_GRPC_STATUS_DELEGATE_H_
#define AISTREAMS_UTIL_GRPC_STATUS_DELEGATE_H_

#include "absl/strings/string_view.h"
#include "aistreams/port/status.h"
#include "grpcpp/grpcpp.h"

namespace aistreams {
namespace util {

// Creates a aistreams::Status from a grpc::Status.
aistreams::Status MakeStatusFromRpcStatus(grpc::Status const& status);

// Creates a aistreams::Status from a grpc::StatusCode and description.
aistreams::Status MakeStatusFromRpcStatus(grpc::StatusCode code,
                                          absl::string_view message);

}  // namespace util
}  // namespace aistreams

#endif  // AISTREAMS_UTIL_GRPC_STATUS_DELEGATE_H_
