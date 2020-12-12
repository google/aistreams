package(
    default_visibility = ["//aistreams:__subpackages__"],
    licenses = ["notice"],  # Apache 2.0
)

cc_library(
    name = "packet",
    srcs = [
        "make_packet.cc",
        "make_packet.h",
        "packet_as.h",
    ],
    hdrs = [
        "packet.h",
    ],
    deps = [
        ":packet_flags",
        "//aistreams/base/types",
        "//aistreams/port:logging",
        "//aistreams/port:status",
        "//aistreams/port:statusor",
        "//aistreams/proto:packet_cc_proto",
        "@com_google_absl//absl/strings",
    ],
)

cc_test(
    name = "packet_test",
    srcs = ["packet_test.cc"],
    deps = [
        ":packet",
        "//aistreams/base/types",
        "//aistreams/port:gtest_main",
        "//aistreams/port:logging",
        "//aistreams/port:status",
        "//aistreams/port:statusor",
        "//aistreams/proto:packet_cc_proto",
        "//aistreams/proto/types:raw_image_cc_proto",
        "//aistreams/proto/types:raw_image_packet_type_descriptor_cc_proto",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
    ],
)

cc_library(
    name = "packet_flags",
    srcs = [
        "packet_flags.cc",
    ],
    hdrs = [
        "packet_flags.h",
    ],
    deps = [
        "//aistreams/port:status",
        "//aistreams/proto:packet_cc_proto",
    ],
)

cc_test(
    name = "packet_flags_test",
    srcs = ["packet_flags_test.cc"],
    deps = [
        ":packet",
        "//aistreams/port:gtest_main",
        "//aistreams/port:logging",
        "//aistreams/port:status",
        "//aistreams/proto:packet_cc_proto",
    ],
)

cc_library(
    name = "connection_options",
    hdrs = ["connection_options.h"],
    deps = [
        "@com_google_absl//absl/time",
    ],
)

cc_library(
    name = "offset_options",
    hdrs = ["offset_options.h"],
    deps = [
        "@com_google_absl//absl/time",
        "@com_google_absl//absl/types:variant",
    ],
)

cc_library(
    name = "stream_channel",
    srcs = ["stream_channel.cc"],
    hdrs = ["stream_channel.h"],
    deps = [
        ":connection_options",
        "//aistreams/base/util:auth_helpers",
        "//aistreams/base/util:grpc_helpers",
        "//aistreams/port:grpc++",
        "//aistreams/port:logging",
        "//aistreams/port:status",
        "//aistreams/port:statusor",
        "//aistreams/util:constants",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
    ],
)

cc_library(
    name = "packet_receiver",
    srcs = ["packet_receiver.cc"],
    hdrs = ["packet_receiver.h"],
    deps = [
        ":connection_options",
        ":offset_options",
        ":stream_channel",
        "//aistreams/port:grpc++",
        "//aistreams/port:logging",
        "//aistreams/port:status",
        "//aistreams/port:statusor",
        "//aistreams/proto:packet_cc_proto",
        "//aistreams/proto:stream_cc_grpc",
        "//aistreams/proto:stream_cc_proto",
        "//aistreams/util:grpc_status_delegate",
        "//aistreams/util:random_string",
        "@com_google_absl//absl/time",
    ],
)

cc_library(
    name = "packet_sender",
    srcs = ["packet_sender.cc"],
    hdrs = ["packet_sender.h"],
    deps = [
        ":connection_options",
        ":stream_channel",
        "//aistreams/port:grpc++",
        "//aistreams/port:logging",
        "//aistreams/port:status",
        "//aistreams/port:statusor",
        "//aistreams/proto:packet_cc_proto",
        "//aistreams/proto:stream_cc_grpc",
        "//aistreams/proto:stream_cc_proto",
        "//aistreams/trace:instrumentation",
    ],
)

cc_library(
    name = "management_client",
    srcs = ["management_client.cc"],
    hdrs = ["management_client.h"],
    deps = [
        "//aistreams/base/util:auth_helpers",
        "//aistreams/base/util:exponential_backoff",
        "//aistreams/base/util:grpc_helpers",
        "//aistreams/port:grpc++",
        "//aistreams/port:logging",
        "//aistreams/port:status",
        "//aistreams/port:statusor",
        "//aistreams/proto:management_cc_grpc",
        "//aistreams/proto:management_cc_proto",
        "@com_google_googleapis//google/longrunning:longrunning_cc_grpc",
        "@com_google_googleapis//google/partner/aistreams/v1alpha1:aistreams_cc_grpc",
    ],
)

cc_test(
    name = "packet_receiver_test",
    srcs = [
        "packet_receiver_test.cc",
    ],
    deps = [
        ":packet_receiver",
        "//aistreams/mocks:mock_stream_service",
        "//aistreams/util:constants",
    ],
)
