package(
    default_visibility = ["//aistreams:__subpackages__"],
    licenses = ["notice"],  # Apache 2.0
)

cc_library(
    name = "types",
    visibility = ["//visibility:public"],
    deps = [
        ":basic_types",
        "//aistreams/base/types/packet_types",
    ],
)

cc_library(
    name = "basic_types",
    hdrs = [
        "basic_types.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":eos",
        ":gstreamer_buffer",
        ":jpeg_frame",
        ":raw_image",
    ],
)

cc_library(
    name = "raw_image",
    srcs = ["raw_image.cc"],
    hdrs = ["raw_image.h"],
    visibility = ["//visibility:public"],
    deps = [
        ":raw_image_helpers",
        "//aistreams/port:logging",
        "//aistreams/port:status",
        "//aistreams/proto/types:raw_image_cc_proto",
    ],
)

cc_library(
    name = "raw_image_helpers",
    srcs = ["raw_image_helpers.cc"],
    hdrs = ["raw_image_helpers.h"],
    deps = [
        "//aistreams/port:logging",
        "//aistreams/port:status",
        "//aistreams/port:statusor",
        "//aistreams/proto/types:raw_image_cc_proto",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
    ],
)

cc_test(
    name = "raw_image_test",
    srcs = ["raw_image_test.cc"],
    deps = [
        ":raw_image",
        ":raw_image_helpers",
        "//aistreams/port:gtest_main",
        "//aistreams/port:logging",
        "//aistreams/port:status",
        "//aistreams/port:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
    ],
)

cc_library(
    name = "jpeg_frame",
    srcs = ["jpeg_frame.cc"],
    hdrs = ["jpeg_frame.h"],
    visibility = ["//visibility:public"],
    deps = [
        "//aistreams/port:logging",
        "//aistreams/port:status",
    ],
)

cc_test(
    name = "jpeg_frame_test",
    srcs = ["jpeg_frame_test.cc"],
    deps = [
        ":jpeg_frame",
        "//aistreams/port:gtest_main",
        "//aistreams/port:logging",
        "//aistreams/port:status",
        "//aistreams/port:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
    ],
)

cc_library(
    name = "gstreamer_buffer",
    hdrs = ["gstreamer_buffer.h"],
    visibility = ["//visibility:public"],
    deps = [
        "//aistreams/port:logging",
        "//aistreams/port:status",
        "@com_google_absl//absl/strings",
    ],
)

cc_test(
    name = "gstreamer_buffer_test",
    srcs = ["gstreamer_buffer_test.cc"],
    deps = [
        ":gstreamer_buffer",
        "//aistreams/port:gtest_main",
        "//aistreams/port:logging",
        "//aistreams/port:status",
        "//aistreams/port:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
    ],
)

cc_library(
    name = "eos",
    hdrs = ["eos.h"],
    visibility = ["//visibility:public"],
    deps = [
        "//aistreams/port:logging",
        "//aistreams/port:status",
        "//aistreams/proto/types:control_signal_cc_proto",
        "@com_google_absl//absl/strings",
    ],
)
