package(
    default_visibility = ["//aistreams:__subpackages__"],
    licenses = ["notice"],  # Apache 2.0
)

cc_library(
    name = "packet_utils",
    srcs = ["packet_utils.cc"],
    hdrs = ["packet_utils.h"],
    deps = [
        "//aistreams/port:logging",
        "//aistreams/port:status",
        "//aistreams/proto:packet_cc_proto",
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
