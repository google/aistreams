package(
    default_visibility = ["//aistreams:__subpackages__"],
    licenses = ["notice"],  # Apache 2.0
)

py_library(
    name = "cli",
    srcs = [
        "commands.py",
    ],
    deps = [
        "//aistreams/python/cli/managed/cluster:cli",
        "//aistreams/python/cli/managed/stream:cli",
    ],
)
