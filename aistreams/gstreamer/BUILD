package(
    default_visibility = ["//aistreams:__subpackages__"],
    licenses = ["notice"],  # Apache 2.0
)

cc_library(
    name = "gstreamer_runner",
    srcs = [
        "gstreamer_runner.cc",
    ],
    hdrs = [
        "gstreamer_runner.h",
    ],
    deps = [
        ":gstreamer_utils",
        "//aistreams/base/types:gstreamer_buffer",
        "//aistreams/port:logging",
        "//aistreams/port:status",
        "//aistreams/port:statusor",
        "//aistreams/util:completion_signal",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_absl//absl/synchronization",
        "@com_google_absl//absl/time",
        "@gstreamer",
    ],
)

cc_test(
    name = "gstreamer_runner_test",
    srcs = ["gstreamer_runner_test.cc"],
    data = ["//testdata:exported_testdata"],
    deps = [
        ":gstreamer_runner",
        ":type_utils",
        "//aistreams/base/types:gstreamer_buffer",
        "//aistreams/port:gtest_main",
        "//aistreams/port:logging",
        "//aistreams/port:status",
        "//aistreams/port:statusor",
        "//aistreams/util:file_helpers",
        "//aistreams/util:producer_consumer_queue",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_absl//absl/time",
        "@gstreamer",
    ],
)

cc_library(
    name = "gstreamer_raw_image_yielder",
    srcs = [
        "gstreamer_raw_image_yielder.cc",
    ],
    hdrs = [
        "gstreamer_raw_image_yielder.h",
    ],
    deps = [
        ":gstreamer_runner",
        ":type_utils",
        "//aistreams/base/types:gstreamer_buffer",
        "//aistreams/base/types:raw_image",
        "//aistreams/port:logging",
        "//aistreams/port:status",
        "//aistreams/port:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
    ],
)

cc_test(
    name = "gstreamer_raw_image_yielder_test",
    srcs = ["gstreamer_raw_image_yielder_test.cc"],
    data = ["//testdata:exported_testdata"],
    deps = [
        ":gstreamer_raw_image_yielder",
        "//aistreams/base/types:gstreamer_buffer",
        "//aistreams/port:gtest_main",
        "//aistreams/port:logging",
        "//aistreams/port:status",
        "//aistreams/port:statusor",
        "//aistreams/util:file_helpers",
        "//aistreams/util:producer_consumer_queue",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_absl//absl/time",
        "@gstreamer",
    ],
)

cc_library(
    name = "gstreamer_video_writer",
    srcs = [
        "gstreamer_video_writer.cc",
    ],
    hdrs = [
        "gstreamer_video_writer.h",
    ],
    deps = [
        ":gstreamer_runner",
        ":type_utils",
        "//aistreams/base/types:gstreamer_buffer",
        "//aistreams/port:logging",
        "//aistreams/port:status",
        "//aistreams/port:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
    ],
)

cc_library(
    name = "gstreamer_video_exporter",
    srcs = [
        "gstreamer_video_exporter.cc",
    ],
    hdrs = [
        "gstreamer_video_exporter.h",
    ],
    deps = [
        ":gstreamer_raw_image_yielder",
        ":gstreamer_runner",
        ":gstreamer_video_writer",
        ":type_utils",
        "//aistreams/base/types:gstreamer_buffer",
        "//aistreams/base/util:packet_utils",
        "//aistreams/base/wrappers:receivers",
        "//aistreams/port:logging",
        "//aistreams/port:status",
        "//aistreams/port:statusor",
        "//aistreams/util:completion_signal",
        "//aistreams/util:file_path",
        "//aistreams/util:producer_consumer_queue",
        "//aistreams/util:random_string",
        "@com_github_googleapis_google_cloud_cpp//:storage_client",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_absl//absl/synchronization",
    ],
)

cc_test(
    name = "gstreamer_video_exporter_test",
    srcs = ["gstreamer_video_exporter_test.cc"],
    data = ["//testdata:exported_testdata"],
    linkopts = [
        "-lstdc++fs",
    ],
    deps = [
        ":gstreamer_video_exporter",
        ":type_utils",
        "//aistreams/base/types:gstreamer_buffer",
        "//aistreams/port:gtest_main",
        "//aistreams/port:logging",
        "//aistreams/port:status",
        "//aistreams/port:statusor",
        "//aistreams/util:file_path",
        "//aistreams/util:producer_consumer_queue",
        "//aistreams/util:random_string",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_absl//absl/time",
    ],
)

cc_library(
    name = "gstreamer_utils",
    srcs = [
        "gstreamer_utils.cc",
    ],
    hdrs = [
        "gstreamer_utils.h",
    ],
    deps = [
        "//aistreams/port:logging",
        "//aistreams/port:status",
        "//aistreams/port:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_absl//absl/time",
        "@gstreamer",
    ],
)

cc_library(
    name = "type_utils",
    srcs = [
        "type_utils.cc",
    ],
    hdrs = [
        "type_utils.h",
    ],
    deps = [
        "//aistreams/base:packet",
        "//aistreams/base/types",
        "//aistreams/base/util:packet_utils",
        "//aistreams/port:logging",
        "//aistreams/port:status",
        "//aistreams/port:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
        "@gstreamer",
    ],
)

cc_library(
    name = "ais_type_utils",
    srcs = [
        "ais_type_utils.cc",
    ],
    hdrs = ["ais_type_utils.h"],
    deps = [
        ":type_utils",
        "//aistreams/c:c_api",
        "//aistreams/port:status",
        "//aistreams/port:statusor",
    ],
)

cc_test(
    name = "type_utils_test",
    srcs = ["type_utils_test.cc"],
    data = ["//testdata:exported_testdata"],
    deps = [
        ":gstreamer_runner",
        ":type_utils",
        "//aistreams/base/types:gstreamer_buffer",
        "//aistreams/port:gtest_main",
        "//aistreams/port:logging",
        "//aistreams/port:status",
        "//aistreams/port:statusor",
        "//aistreams/util:file_helpers",
        "//aistreams/util:producer_consumer_queue",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_absl//absl/time",
        "@gstreamer",
    ],
)
