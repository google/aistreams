cc_binary(
    name = "libaissink.so",
    srcs = [
        "aissink.c",
        "aissink.h",
    ],
    linkshared = 1,
    visibility = ["//aistreams:__subpackages__"],
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
    visibility = ["//aistreams:__subpackages__"],
    deps = [
        "//aistreams/c:c_api",
        "//aistreams/gstreamer:ais_type_utils",
        "@gstreamer",
    ],
)
