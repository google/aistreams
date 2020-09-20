package(
    default_visibility = ["//aistreams:__subpackages__"],
    licenses = ["notice"],  # Apache 2.0
)

py_library(
    name = "cli",
    srcs = [
        "commands.py",
    ],
    data = [
        "//aistreams/gstreamer/gst-plugins:libaissink.so",
        "//aistreams/python/apps:ingester_app",
    ],
    deps = [
        "//aistreams/python/apps",
        "//aistreams/python/cli:util",
    ],
)
