package(default_visibility = ["//visibility:public"])

load("@rules_proto//proto:defs.bzl", "proto_library")
load("@com_github_grpc_grpc//bazel:cc_grpc_library.bzl", "cc_grpc_library")

proto_library(
    name = "management_proto",
    srcs = ["management.proto"],
    deps = [
        "@com_google_protobuf//:any_proto",
        "@com_google_protobuf//:empty_proto",
    ],
)

cc_proto_library(
    name = "management_cc_proto",
    deps = [":management_proto"],
)

cc_grpc_library(
    name = "management_cc_grpc",
    srcs = [":management_proto"],
    grpc_only = True,
    well_known_protos = True,
    deps = [":management_cc_proto"],
)

proto_library(
    name = "packet_proto",
    srcs = ["packet.proto"],
    deps = [
        "//aistreams/proto/types:packet_type_proto",
        "@com_google_protobuf//:any_proto",
        "@com_google_protobuf//:timestamp_proto",
    ],
)

cc_proto_library(
    name = "packet_cc_proto",
    visibility = ["//visibility:public"],
    deps = [":packet_proto"],
)

proto_library(
    name = "stream_proto",
    srcs = ["stream.proto"],
    deps = [
        ":packet_proto",
        "@com_google_protobuf//:any_proto",
        "@com_google_protobuf//:empty_proto",
    ],
)

cc_proto_library(
    name = "stream_cc_proto",
    deps = [":stream_proto"],
)

cc_grpc_library(
    name = "stream_cc_grpc",
    srcs = [":stream_proto"],
    grpc_only = True,
    well_known_protos = True,
    deps = [":stream_cc_proto"],
)
