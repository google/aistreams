package(
    default_visibility = ["//aistreams:__subpackages__"],
    licenses = ["notice"],  # Apache 2.0
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
