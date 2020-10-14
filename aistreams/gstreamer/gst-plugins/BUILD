package(
    default_visibility = ["//aistreams:__subpackages__"],
    licenses = ["notice"],  #Apache 2.0
)

cc_binary(
    name = "libaissink.so",
    srcs = [
        "aissink.c",
        "aissink.h",
    ],
    linkshared = 1,
    visibility = ["//visibility:public"],
    deps = [
        "//aistreams/c:c_api",
        "@gstreamer",
    ],
)

cc_binary(
    name = "libaissrc.so",
    srcs = [
        "aissrc.c",
        "aissrc.h",
    ],
    linkshared = 1,
    visibility = ["//visibility:public"],
    deps = [
        "//aistreams/c:c_api",
        "//aistreams/gstreamer:ais_type_utils",
        "@gstreamer",
    ],
)
