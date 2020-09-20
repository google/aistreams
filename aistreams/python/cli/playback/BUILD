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
        "//aistreams/gstreamer/gst-plugins:libaissrc.so",
        "//aistreams/python/apps:playback_app",
    ],
    deps = [
        "//aistreams/python/apps",
        "//aistreams/python/cli:util",
    ],
)
