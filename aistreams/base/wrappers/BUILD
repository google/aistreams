package(
    default_visibility = ["//aistreams:__subpackages__"],
    licenses = ["notice"],  # Apache 2.0
)

cc_library(
    name = "receivers",
    srcs = [
        "receivers.cc",
    ],
    hdrs = [
        "receivers.h",
        "receiver_queue.h",
    ],
    deps = [
        "//aistreams/base:connection_options",
        "//aistreams/base:packet",
        "//aistreams/base:packet_receiver",
        "//aistreams/base/types:basic_types",
        "//aistreams/port:logging",
        "//aistreams/port:status",
        "//aistreams/port:statusor",
        "//aistreams/util:producer_consumer_queue",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/time",
    ],
)
