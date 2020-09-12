package(
    default_visibility = ["//aistreams:__subpackages__"],
    licenses = ["notice"],  # Apache 2.0
)

py_binary(
    name = "aisctl",
    srcs = [
        "aisctl.py",
    ],
    data = [
        ":ingester_app",
        "//aistreams/gstreamer/gst-plugins:libaissink.so",
    ],
)

cc_binary(
    name = "ingester_app",
    srcs = [
        "ingester_app.cc",
    ],
    deps = [
        "//aistreams/cc:aistreams",
        "//aistreams/port:logging",
        "//aistreams/port:status",
        "//aistreams/port:statusor",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/flags:parse",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/time",
    ],
)
