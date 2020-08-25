package(
    default_visibility = ["//aistreams:__subpackages__"],
    licenses = ["notice"],  # Apache 2.0
)

cc_library(
    name = "packet_types",
    srcs = [
        "eos_packet_type.cc",
        "eos_packet_type.h",
        "gstreamer_buffer_packet_type.cc",
        "gstreamer_buffer_packet_type.h",
        "jpeg_frame_packet_type.cc",
        "jpeg_frame_packet_type.h",
        "packet_type_traits.h",
        "protobuf_packet_type.h",
        "raw_image_packet_type.cc",
        "raw_image_packet_type.h",
        "string_packet_type.cc",
        "string_packet_type.h",
    ],
    hdrs = [
        "packet_types.h",
    ],
    deps = [
        "//aistreams/base/types:basic_types",
        "//aistreams/port:logging",
        "//aistreams/port:status",
        "//aistreams/port:statusor",
        "//aistreams/proto:packet_cc_proto",
        "//aistreams/proto/types:control_signal_packet_type_descriptor_cc_proto",
        "//aistreams/proto/types:gstreamer_buffer_packet_type_descriptor_cc_proto",
        "//aistreams/proto/types:packet_type_cc_proto",
        "//aistreams/proto/types:protobuf_packet_type_descriptor_cc_proto",
        "//aistreams/proto/types:raw_image_cc_proto",
        "//aistreams/proto/types:raw_image_packet_type_descriptor_cc_proto",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
    ],
)
