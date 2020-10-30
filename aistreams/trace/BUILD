package(default_visibility = ["//aistreams:__subpackages__"])

cc_library(
    name = "instrumentation",
    srcs = [
        "instrumentation.cc",
    ],
    hdrs = [
        "instrumentation.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//aistreams/port:logging",
        "//aistreams/proto:packet_cc_proto",
        "//aistreams/util:constants",
        "@com_census_instrumentation_cpp//opencensus/trace",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/strings:str_format",
    ],
)

cc_test(
    name = "instrumentation_test",
    srcs = ["instrumentation_test.cc"],
    deps = [
        ":instrumentation",
        "//aistreams/port:gtest_main",
    ],
)
