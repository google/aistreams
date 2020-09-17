package(
    default_visibility = ["//aistreams:__subpackages__"],
    licenses = ["notice"],  # Apache 2.0
)

cc_binary(
    name = "libaissink.so",
    srcs = [
        "aissink.c",
        "aissink.h",
    ],
    linkshared = 1,
    deps = [
        "//aistreams/c:c_api",
        "@gstreamer",
    ],
)

cc_binary(
    name = "libaissrc.so",
    srcs = [
        "aissrc.c",
        "aissrc.h",
    ],
    linkshared = 1,
    deps = [
        "//aistreams/c:c_api",
        "//aistreams/gstreamer:ais_type_utils",
        "@gstreamer",
    ],
)

cc_library(
    name = "cli_builders",
    srcs = [
        "aissink_cli_builder.cc",
        "aissrc_cli_builder.cc",
    ],
    hdrs = [
        "aissink_cli_builder.h",
        "aissrc_cli_builder.h",
    ],
    deps = [
        "//aistreams/base:connection_options",
        "//aistreams/port:logging",
        "//aistreams/port:status",
        "//aistreams/port:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
    ],
)
