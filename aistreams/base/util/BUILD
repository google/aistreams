package(
    default_visibility = ["//aistreams:__subpackages__"],
    licenses = ["notice"],  # Apache 2.0
)

cc_library(
    name = "packet_utils",
    srcs = ["packet_utils.cc"],
    hdrs = ["packet_utils.h"],
    deps = [
        "//aistreams/base:packet",
        "//aistreams/port:logging",
        "//aistreams/port:status",
        "//aistreams/port:statusor",
        "//aistreams/proto:packet_cc_proto",
        "//aistreams/proto/types:control_signal_cc_proto",
        "//aistreams/proto/types:control_signal_packet_type_descriptor_cc_proto",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_test(
    name = "packet_utils_test",
    srcs = ["packet_utils_test.cc"],
    deps = [
        ":packet_utils",
        "//aistreams/base:packet",
        "//aistreams/base/types",
        "//aistreams/port:gtest_main",
        "//aistreams/port:logging",
        "//aistreams/port:status",
        "//aistreams/port:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
    ],
)

cc_library(
    name = "grpc_helpers",
    srcs = ["grpc_helpers.cc"],
    hdrs = ["grpc_helpers.h"],
    deps = [
        "//aistreams/base:connection_options",
        "//aistreams/port:grpc++",
        "//aistreams/port:logging",
        "//aistreams/port:status",
        "//aistreams/port:statusor",
        "//aistreams/util:file_helpers",
        "@com_google_absl//absl/strings",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_test(
    name = "grpc_helpers_test",
    srcs = [
        "grpc_helpers_test.cc",
    ],
    deps = [
        ":grpc_helpers",
        "//aistreams/port:gtest_main",
    ],
)

cc_library(
    name = "raw_image_utils",
    srcs = ["raw_image_utils.cc"],
    hdrs = ["raw_image_utils.h"],
    deps = [
        "//aistreams/base/types:raw_image",
        "//aistreams/port:logging",
        "//aistreams/port:status",
        "//aistreams/proto/types:raw_image_cc_proto",
        "//aistreams/util:file_helpers",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
    ],
)

cc_library(
    name = "auth_helpers",
    srcs = ["auth_helpers.cc"],
    hdrs = ["auth_helpers.h"],
    deps = [
        ":grpc_helpers",
        "@com_github_tencent_rapidjson//:rapidjson",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_googleapis//google/iam/credentials/v1:credentials_cc_grpc",
    ],
)

cc_library(
    name = "exponential_backoff",
    srcs = ["exponential_backoff.cc"],
    hdrs = ["exponential_backoff.h"],
    deps = [
        "@com_google_absl//absl/time",
    ],
)
