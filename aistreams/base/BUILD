package(
    default_visibility = ["//aistreams:__subpackages__"],
    licenses = ["notice"],  # Apache 2.0
)

cc_library(
    name = "packet",
    srcs = [
        "make_packet.h",
        "packet_as.h",
    ],
    hdrs = [
        "packet.h",
    ],
    deps = [
        ":packet_utils",
        "//aistreams/base/types",
        "//aistreams/port:logging",
        "//aistreams/port:status",
        "//aistreams/port:statusor",
        "//aistreams/proto:packet_cc_proto",
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
    name = "packet_utils",
    srcs = ["packet_utils.cc"],
    hdrs = ["packet_utils.h"],
    deps = [
        "//aistreams/port:logging",
        "//aistreams/port:status",
        "//aistreams/proto:packet_cc_proto",
    ],
)
