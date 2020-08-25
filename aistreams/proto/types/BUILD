package(default_visibility = ["//visibility:public"])

load("@rules_proto//proto:defs.bzl", "proto_library")

proto_library(
    name = "packet_type_proto",
    srcs = ["packet_type.proto"],
    deps = [
        "@com_google_protobuf//:any_proto",
    ],
)

cc_proto_library(
    name = "packet_type_cc_proto",
    deps = [":packet_type_proto"],
)

proto_library(
    name = "raw_image_proto",
    srcs = ["raw_image.proto"],
)

cc_proto_library(
    name = "raw_image_cc_proto",
    deps = [":raw_image_proto"],
)

proto_library(
    name = "raw_image_packet_type_descriptor_proto",
    srcs = ["raw_image_packet_type_descriptor.proto"],
    deps = [
        ":raw_image_proto",
        "@com_google_protobuf//:any_proto",
    ],
)

cc_proto_library(
    name = "raw_image_packet_type_descriptor_cc_proto",
    deps = [":raw_image_packet_type_descriptor_proto"],
)

proto_library(
    name = "protobuf_packet_type_descriptor_proto",
    srcs = ["protobuf_packet_type_descriptor.proto"],
    deps = [
        "@com_google_protobuf//:any_proto",
    ],
)

cc_proto_library(
    name = "protobuf_packet_type_descriptor_cc_proto",
    deps = [":protobuf_packet_type_descriptor_proto"],
)

proto_library(
    name = "gstreamer_buffer_packet_type_descriptor_proto",
    srcs = ["gstreamer_buffer_packet_type_descriptor.proto"],
    deps = [
        "@com_google_protobuf//:any_proto",
    ],
)

cc_proto_library(
    name = "gstreamer_buffer_packet_type_descriptor_cc_proto",
    deps = [":gstreamer_buffer_packet_type_descriptor_proto"],
)

proto_library(
    name = "control_signal_proto",
    srcs = ["control_signal.proto"],
)

cc_proto_library(
    name = "control_signal_cc_proto",
    deps = [":control_signal_proto"],
)

proto_library(
    name = "control_signal_packet_type_descriptor_proto",
    srcs = ["control_signal_packet_type_descriptor.proto"],
    deps = [
        ":control_signal_proto",
        "@com_google_protobuf//:any_proto",
    ],
)

cc_proto_library(
    name = "control_signal_packet_type_descriptor_cc_proto",
    deps = [":control_signal_packet_type_descriptor_proto"],
)
