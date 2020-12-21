package_group(
    name = "trusted_projects",
    packages = [
        "//aistreams/port/...",
    ],
)

package(default_visibility = [":trusted_projects"])

cc_library(
    name = "status",
    srcs = [
        "status.cc",
        "status_builder.cc",
    ],
    hdrs = [
        "canonical_errors.h",
        "status.h",
        "status_builder.h",
        "status_macros.h",
    ],
    deps = [
        ":source_location",
        "//aistreams/port:logging",
        "//aistreams/util:examine_stack",
        "@com_google_absl//absl/base:core_headers",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_absl//absl/synchronization",
    ],
)

cc_library(
    name = "source_location",
    hdrs = ["source_location.h"],
)

cc_library(
    name = "status_matchers",
    testonly = 1,
    hdrs = ["status_matchers.h"],
    deps = [
        ":status",
    ],
)

cc_library(
    name = "ret_check",
    srcs = ["ret_check.cc"],
    hdrs = ["ret_check.h"],
    deps = [
        ":status",
        "@com_google_absl//absl/base:core_headers",
    ],
)

cc_library(
    name = "statusor",
    srcs = ["statusor.cc"],
    hdrs = [
        "statusor.h",
        "statusor_internals.h",
    ],
    deps = [
        ":status",
        "@com_google_absl//absl/base:core_headers",
    ],
)

cc_test(
    name = "status_builder_test",
    size = "small",
    srcs = ["status_builder_test.cc"],
    linkstatic = 1,
    deps = [
        ":status",
        ":statusor",
        "//aistreams/port:gtest_main",
    ],
)

cc_test(
    name = "status_test",
    size = "small",
    srcs = ["status_test.cc"],
    linkstatic = 1,
    deps = [
        ":status",
        ":status_matchers",
        "//aistreams/port:gtest_main",
    ],
)

cc_test(
    name = "statusor_test",
    size = "small",
    srcs = ["statusor_test.cc"],
    linkstatic = 1,
    deps = [
        ":status",
        ":statusor",
        "//aistreams/port:gtest_main",
    ],
)

cc_test(
    name = "status_macros_test",
    srcs = [
        "status_macros_test.cc",
    ],
    deps = [
        ":status",
        ":statusor",
        "//aistreams/port:gtest_main",
    ],
)
