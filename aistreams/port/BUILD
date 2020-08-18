cc_library(
    name = "gtest_main",
    testonly = 1,
    hdrs = [
        "gmock.h",
        "gtest.h",
        "status_matchers.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//aistreams/util/status:status_matchers",
        "@com_github_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "benchmark",
    hdrs = [
        "benchmark.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "@com_github_google_benchmark//:benchmark",
        "@com_github_google_benchmark//:benchmark_main",
    ],
)

cc_library(
    name = "grpc++",
    hdrs = [
        "grpcpp.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "@com_github_grpc_grpc//:grpc++",
    ],
)

cc_library(
    name = "logging",
    hdrs = [
        "logging.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "@com_github_google_glog//:glog",
    ],
)

cc_library(
    name = "ret_check",
    hdrs = [
        "ret_check.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//aistreams/util/status:ret_check",
    ],
)

cc_library(
    name = "source_location",
    hdrs = [
        "source_location.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//aistreams/util/status:source_location",
    ],
)

cc_library(
    name = "status",
    hdrs = [
        "canonical_errors.h",
        "status.h",
        "status_builder.h",
        "status_macros.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":source_location",
        "//aistreams/util/status",
        "@com_google_absl//absl/strings",
    ],
)

cc_library(
    name = "statusor",
    hdrs = [
        "statusor.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//aistreams/util/status:statusor",
    ],
)
