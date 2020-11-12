package(default_visibility = ["//visibility:public"])

load("@rules_proto//proto:defs.bzl", "proto_library")
load("@io_bazel_rules_go//go:def.bzl", "go_library")
load("@io_bazel_rules_go//proto:def.bzl", "go_proto_library")
load("@com_github_grpc_grpc//bazel:cc_grpc_library.bzl", "cc_grpc_library")

proto_library(
    name = "management_proto",
    srcs = ["management.proto"],
    deps = [
        "@com_google_protobuf//:any_proto",
        "@com_google_protobuf//:duration_proto",
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
        "@com_google_protobuf//:duration_proto",
        "@com_google_protobuf//:timestamp_proto",
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

go_proto_library(
    name = "management_go_proto",
    compilers = ["@io_bazel_rules_go//proto:go_grpc"],
    importpath = "aistreams/proto",
    proto = ":management_proto",
    visibility = ["//visibility:public"],
)

go_library(
    name = "go_default_library",
    embed = [":management_go_proto"],
    importpath = "aistreams/proto",
    visibility = ["//visibility:public"],
)
