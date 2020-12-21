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
    visibility = ["//visibility:public"],
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

cc_library(
    name = "constants",
    hdrs = [
        "constants.h",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "grpc_status_delegate",
    srcs = ["grpc_status_delegate.cc"],
    hdrs = ["grpc_status_delegate.h"],
    visibility = ["//visibility:public"],
    deps = [
        "//aistreams/port:grpc++",
        "//aistreams/port:status",
        "@com_google_absl//absl/strings",
    ],
)

cc_test(
    name = "grpc_status_delegate_test",
    srcs = ["grpc_status_delegate_test.cc"],
    deps = [
        ":grpc_status_delegate",
        "//aistreams/port:gtest_main",
    ],
)

cc_library(
    name = "completion_signal",
    srcs = [
        "completion_signal.cc",
    ],
    hdrs = [
        "completion_signal.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//aistreams/port:status",
        "@com_google_absl//absl/synchronization",
        "@com_google_absl//absl/time",
    ],
)

cc_test(
    name = "completion_signal_test",
    srcs = ["completion_signal_test.cc"],
    deps = [
        ":completion_signal",
        "//aistreams/port:gtest_main",
        "//aistreams/port:status",
    ],
)

cc_library(
    name = "random_string",
    srcs = [
        "random_string.cc",
    ],
    hdrs = [
        "random_string.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//aistreams/port:status",
        "@com_google_absl//absl/random",
    ],
)

cc_library(
    name = "examine_stack",
    srcs = [
        "examine_stack.cc",
    ],
    hdrs = [
        "examine_stack.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "@com_google_absl//absl/debugging:stacktrace",
        "@com_google_absl//absl/debugging:symbolize",
    ],
)

cc_test(
    name = "examine_stack_test",
    srcs = [
        "examine_stack_test.cc",
    ],
    deps = [
        ":examine_stack",
        "//aistreams/port:gtest_main",
        "@com_google_absl//absl/strings",
    ],
)
