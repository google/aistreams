licenses(["notice"])

exports_files(["LICENSE"])

cc_library(
    name = "benchmark",
    srcs = glob([
        "src/*.h",
        "src/*.cc",
    ]),
    hdrs = glob(["include/benchmark/*.h"]),
    copts = ["-DHAVE_POSIX_REGEX"],
    includes = ["include"],
    visibility = ["//visibility:public"],
)
