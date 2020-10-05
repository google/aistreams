package(
    default_visibility = ["//aistreams:__subpackages__"],
    licenses = ["notice"],  # Apache 2.0
)

cc_library(
    name = "managers",
    srcs = ["managers.cc"],
    hdrs = ["managers.h"],
    deps = [
        "//aistreams/base:management_client",
        "//aistreams/port:logging",
        "//aistreams/port:status",
        "//aistreams/port:statusor",
    ],
)

cc_library(
    name = "receivers",
    srcs = [
        "receivers.cc",
    ],
    hdrs = [
        "receiver_queue.h",
        "receivers.h",
    ],
    deps = [
        "//aistreams/base:connection_options",
        "//aistreams/base:offset_options",
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

cc_library(
    name = "senders",
    srcs = [
        "senders.cc",
    ],
    hdrs = [
        "senders.h",
    ],
    deps = [
        "//aistreams/base:connection_options",
        "//aistreams/base:packet",
        "//aistreams/base:packet_sender",
        "//aistreams/base/types:basic_types",
        "//aistreams/port:logging",
        "//aistreams/port:status",
        "//aistreams/port:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/time",
    ],
)
