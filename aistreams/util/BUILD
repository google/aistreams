package(default_visibility = ["//aistreams:__subpackages__"])

cc_library(
    name = "file_path",
    srcs = ["file_path.cc"],
    hdrs = ["file_path.h"],
    visibility = ["//visibility:public"],
    deps = [
        "@com_google_absl//absl/strings",
    ],
)

cc_library(
    name = "file_helpers",
    srcs = ["file_helpers.cc"],
    hdrs = ["file_helpers.h"],
    visibility = ["//visibility:public"],
    deps = [
        ":file_path",
        "//aistreams/port:status",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
    ],
)

cc_library(
    name = "producer_consumer_queue",
    hdrs = [
        "producer_consumer_queue.h",
    ],
    deps = [
        "//aistreams/port:logging",
        "@com_google_absl//absl/synchronization",
        "@com_google_absl//absl/time",
    ],
)

cc_test(
    name = "producer_consumer_queue_test",
    srcs = ["producer_consumer_queue_test.cc"],
    linkstatic = 1,
    deps = [
        ":producer_consumer_queue",
        "//aistreams/port:gtest_main",
    ],
)
