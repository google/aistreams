package(
    default_visibility = ["//aistreams:__subpackages__"],
    licenses = ["notice"],  # Apache 2.0
)

py_library(
    name = "apps",
    srcs = [
        "__init__.py",
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

cc_binary(
    name = "manager_app",
    srcs = [
        "manager_app.cc",
    ],
    deps = [
        "//aistreams/cc:aistreams",
        "//aistreams/port:logging",
        "//aistreams/port:status",
        "//aistreams/port:statusor",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/flags:parse",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/time",
    ],
)

cc_binary(
    name = "playback_app",
    srcs = [
        "playback_app.cc",
    ],
    deps = [
        "//aistreams/base:connection_options",
        "//aistreams/gstreamer:gstreamer_utils",
        "//aistreams/gstreamer/gst-plugins/cli_builders",
        "//aistreams/port:logging",
        "//aistreams/port:status",
        "//aistreams/port:statusor",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/flags:parse",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/time",
    ],
)

cc_binary(
    name = "printer_app",
    srcs = [
        "printer_app.cc",
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

cc_binary(
    name = "replay_app",
    srcs = [
        "replay_app.cc",
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

cc_binary(
    name = "chunker_app",
    srcs = [
        "chunker_app.cc",
    ],
    deps = [
        "//aistreams/base/wrappers:receivers",
        "//aistreams/gstreamer:gstreamer_video_exporter",
        "//aistreams/port:logging",
        "//aistreams/port:status",
        "//aistreams/port:statusor",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/flags:parse",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_absl//absl/time",
    ],
)
