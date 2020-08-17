package(
    default_visibility = ["//aistreams:__subpackages__"],
    licenses = ["notice"],  # Apache 2.0
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
        "//aistreams/base:packet_receiver",
        "//aistreams/base:packet_sender",
        "//aistreams/base/types",
        "//aistreams/proto:packet_cc_proto",
    ],
)
