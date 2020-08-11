workspace(name = "com_google_aistreams_sdk")

load(
    "//bazel:aistreams_sdk_deps.bzl",
    "aistreams_sdk_deps",
)

aistreams_sdk_deps()

load("@com_github_grpc_grpc//bazel:grpc_deps.bzl", "grpc_deps")

grpc_deps()

# Begin workaround needed for cc_grpc_library
load("@upb//bazel:workspace_deps.bzl", "upb_deps")

upb_deps()

load(
    "@build_bazel_rules_apple//apple:repositories.bzl",
    "apple_rules_dependencies",
)

apple_rules_dependencies()

load(
    "@build_bazel_apple_support//lib:repositories.bzl",
    "apple_support_dependencies",
)

apple_support_dependencies()
# End workaround for cc_grpc_library
