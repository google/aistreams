package(
    default_visibility = ["//aistreams:__subpackages__"],
    licenses = ["notice"],  # Apache 2.0
)

cc_library(
    name = "aistreams",
    hdrs = [
        "aistreams.h",
    ],
    data = [
        "//aistreams/gstreamer/gst-plugins:libaissink.so",
        "//aistreams/gstreamer/gst-plugins:libaissrc.so",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":aistreams_lite",
        ":decoded_receivers",
        ":ingesters",
    ],
)

cc_library(
    name = "aistreams_lite",
    hdrs = [
        "aistreams_lite.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//aistreams/base:connection_options",
        "//aistreams/base:management_client",
        "//aistreams/base:packet",
        "//aistreams/base/types",
        "//aistreams/base/util:packet_utils",
        "//aistreams/base/wrappers:receivers",
        "//aistreams/base/wrappers:senders",
        "//aistreams/proto:packet_cc_proto",
    ],
)

cc_library(
    name = "decoded_receivers",
    srcs = [
        "decoded_receivers.cc",
    ],
    hdrs = [
        "decoded_receivers.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//aistreams/base/types",
        "//aistreams/cc:aistreams_lite",
        "//aistreams/gstreamer:gstreamer_raw_image_yielder",
        "//aistreams/gstreamer:type_utils",
        "//aistreams/port:logging",
        "//aistreams/port:status",
        "//aistreams/port:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/time",
    ],
)

cc_library(
    name = "ingesters",
    srcs = [
        "ingesters.cc",
    ],
    hdrs = [
        "ingesters.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//aistreams/cc:aistreams_lite",
        "//aistreams/gstreamer:gstreamer_utils",
        "//aistreams/gstreamer/gst-plugins/cli_builders",
        "//aistreams/port:logging",
        "//aistreams/port:status",
        "//aistreams/port:statusor",
        "//aistreams/util:file_helpers",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_absl//absl/time",
    ],
)
