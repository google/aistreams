package(
    default_visibility = ["//aistreams:__subpackages__"],
)

cc_library(
    name = "c_api",
    srcs = [
        "c_api.cc",
        "c_api_internal.h",
    ],
    hdrs = ["c_api.h"],
    visibility = ["//visibility:public"],
    deps = [
        ":ais_gstreamer_buffer",
        ":ais_logging",
        ":ais_packet",
        ":ais_packet_as",
        ":ais_status",
        "//aistreams/base:connection_options",
        "//aistreams/base/wrappers:receivers",
        "//aistreams/base/wrappers:senders",
        "//aistreams/port:status",
        "//aistreams/port:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/time",
    ],
)

cc_library(
    name = "ais_status",
    srcs = [
        "ais_status.cc",
        "ais_status_internal.h",
    ],
    hdrs = ["ais_status.h"],
    visibility = ["//visibility:public"],
    deps = [
        "//aistreams/port:status",
    ],
)

cc_library(
    name = "ais_logging",
    srcs = [
        "ais_logging.cc",
    ],
    hdrs = ["ais_logging.h"],
    visibility = ["//visibility:public"],
    deps = [
        "//aistreams/port:logging",
    ],
)

cc_library(
    name = "ais_packet",
    srcs = [
        "ais_packet.cc",
        "ais_packet_internal.h",
    ],
    hdrs = ["ais_packet.h"],
    visibility = ["//visibility:public"],
    deps = [
        ":ais_gstreamer_buffer",
        ":ais_status",
        "//aistreams/base:packet",
        "//aistreams/base/types:gstreamer_buffer",
        "//aistreams/base/util:packet_utils",
        "//aistreams/port:status",
        "//aistreams/proto:packet_cc_proto",
        "//aistreams/proto/types:packet_type_cc_proto",
    ],
)

cc_library(
    name = "ais_gstreamer_buffer",
    srcs = [
        "ais_gstreamer_buffer.cc",
        "ais_gstreamer_buffer_internal.h",
    ],
    hdrs = ["ais_gstreamer_buffer.h"],
    visibility = ["//visibility:public"],
    deps = [
        ":ais_status",
        "//aistreams/base/types:gstreamer_buffer",
        "//aistreams/port:status",
    ],
)

cc_library(
    name = "ais_packet_as",
    srcs = [
        "ais_packet_as.cc",
        "ais_packet_as_internal.h",
    ],
    hdrs = ["ais_packet_as.h"],
    visibility = ["//visibility:public"],
    deps = [
        ":ais_gstreamer_buffer",
        ":ais_packet",
        ":ais_status",
        "//aistreams/base:packet",
        "//aistreams/base/types:gstreamer_buffer",
        "//aistreams/port:status",
        "//aistreams/proto:packet_cc_proto",
    ],
)

cc_test(
    name = "ais_gstreamer_buffer_test",
    srcs = ["ais_gstreamer_buffer_test.cc"],
    deps = [
        ":ais_gstreamer_buffer",
        "//aistreams/base/types:gstreamer_buffer",
        "//aistreams/port:gtest_main",
        "//aistreams/port:logging",
        "//aistreams/port:status",
        "//aistreams/port:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
    ],
)

cc_test(
    name = "ais_packet_test",
    srcs = ["ais_packet_test.cc"],
    deps = [
        ":ais_gstreamer_buffer",
        ":ais_packet",
        ":ais_packet_as",
        ":ais_status",
        "//aistreams/port:gtest_main",
        "//aistreams/port:logging",
        "//aistreams/port:status",
        "//aistreams/port:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
    ],
)

cc_test(
    name = "c_api_test",
    srcs = ["c_api_test.cc"],
    deps = [
        ":c_api",
        "//aistreams/port:gtest_main",
        "//aistreams/port:logging",
        "//aistreams/port:status",
        "//aistreams/port:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
    ],
)
