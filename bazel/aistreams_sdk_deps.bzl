"""Load dependencies needed to compile and test aistreams-sdk as a 3rd-party consumer."""

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@bazel_tools//tools/build_defs/repo:utils.bzl", "maybe")

def aistreams_sdk_deps():
    """Declare and load direct dependencies for aistreams-sdk."""

    maybe(
        http_archive,
        name = "com_github_grpc_grpc",
        strip_prefix = "grpc-1.29.1",
        urls = [
            "https://github.com/grpc/grpc/archive/v1.29.1.tar.gz",
        ],
        sha256 = "0343e6dbde66e9a31c691f2f61e98d79f3584e03a11511fad3f10e3667832a45",
    )

    maybe(
        http_archive,
        name = "com_google_absl",
        strip_prefix = "abseil-cpp-20190808",
        urls = [
            "https://github.com/abseil/abseil-cpp/archive/20190808.tar.gz",
        ],
        sha256 = "8100085dada279bf3ee00cd064d43b5f55e5d913be0dfe2906f06f8f28d5b37e",
    )

    maybe(
        http_archive,
        name = "com_github_google_glog",
        strip_prefix = "glog-0.4.0",
        urls = [
            "https://github.com/google/glog/archive/v0.4.0.tar.gz",
        ],
        sha256 = "f28359aeba12f30d73d9e4711ef356dc842886968112162bc73002645139c39c",
    )

    maybe(
        http_archive,
        name = "com_github_google_benchmark",
        urls = [
            "https://github.com/google/benchmark/archive/v1.5.1.tar.gz",
        ],
        strip_prefix = "benchmark-1.5.1",
        sha256 = "23082937d1663a53b90cb5b61df4bcc312f6dee7018da78ba00dd6bd669dfef2",
    )

    maybe(
        http_archive,
        name = "com_github_google_googletest",
        strip_prefix = "googletest-release-1.8.1",
        urls = [
            "https://github.com/google/googletest/archive/release-1.8.1.tar.gz",
        ],
        sha256 = "9bf1fe5182a604b4135edc1a425ae356c9ad15e9b23f9f12a02e80184c3a249c",
    )

    maybe(
        http_archive,
        name = "com_google_protobuf",
        sha256 = "416212e14481cff8fd4849b1c1c1200a7f34808a54377e22d7447efdf54ad758",
        strip_prefix = "protobuf-09745575a923640154bcf307fba8aedff47f240a",
        url = "https://github.com/google/protobuf/archive/09745575a923640154bcf307fba8aedff47f240a.tar.gz",  # 2019-08-19
    )

    # This requires gstreamer to be installed on your system.
    # TODO: Ok to start, but consider building from source with bazel.
    native.new_local_repository(
        name = "gstreamer",
        build_file = "//third_party:gstreamer.BUILD",
        path = "/usr",
    )
