package(
    default_visibility = ["//aistreams:__subpackages__"],
    licenses = ["notice"],  # Apache 2.0
)

cc_library(
    name = "receiver_queue",
    hdrs = [
        "receiver_queue.h",
    ],
    deps = [
        "//aistreams/port:logging",
        "//aistreams/port:status",
        "//aistreams/port:statusor",
        "//aistreams/util:producer_consumer_queue",
        "@com_google_absl//absl/strings",
    ],
)
