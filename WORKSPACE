workspace(name = "com_google_aistreams_sdk")

load("//bazel:aistreams_sdk_deps.bzl", "aistreams_sdk_deps")

aistreams_sdk_deps()

# Requirements for @com_github_grpc_grpc.
load("@com_github_grpc_grpc//bazel:grpc_deps.bzl", "grpc_deps")

grpc_deps()

load("@com_github_grpc_grpc//bazel:grpc_extra_deps.bzl", "grpc_extra_deps")

grpc_extra_deps()
