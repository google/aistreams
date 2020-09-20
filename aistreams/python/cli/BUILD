package(
    default_visibility = ["//aistreams:__subpackages__"],
    licenses = ["notice"],  # Apache 2.0
)

py_binary(
    name = "aisctl",
    srcs = [
        "aisctl.py",
    ],
    deps = [
        "//aistreams/python/cli/ingest:cli",
        "//aistreams/python/cli/managed:cli",
        "//aistreams/python/cli/onprem:cli",
        "//aistreams/python/cli/playback:cli",
    ],
)

py_binary(
    name = "util",
    srcs = [
        "util.py",
    ],
)
