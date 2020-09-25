proto_library(
    name = "hello_proto",
    srcs = ["hello.proto"],
)

cc_proto_library(
    name = "hello_cc_proto",
    deps = [":hello_proto"],
)

cc_binary(
    name = "simple_sender_main",
    srcs = [
        "simple_sender_main.cc",
    ],
    deps = [
        ":hello_cc_proto",
        "@com_github_google_aistreams//aistreams/cc:aistreams",
        "@com_github_google_aistreams//aistreams/port:logging",
        "@com_github_google_aistreams//aistreams/port:status",
        "@com_github_google_aistreams//aistreams/port:statusor",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/flags:parse",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/time",
    ],
)

cc_binary(
    name = "simple_receiver_main",
    srcs = [
        "simple_receiver_main.cc",
    ],
    deps = [
        ":hello_cc_proto",
        "@com_github_google_aistreams//aistreams/cc:aistreams",
        "@com_github_google_aistreams//aistreams/port:logging",
        "@com_github_google_aistreams//aistreams/port:status",
        "@com_github_google_aistreams//aistreams/port:statusor",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/flags:parse",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/time",
    ],
)

cc_binary(
    name = "simple_ingester_main",
    srcs = [
        "simple_ingester_main.cc",
    ],
    deps = [
        "@com_github_google_aistreams//aistreams/cc:aistreams",
        "@com_github_google_aistreams//aistreams/port:logging",
        "@com_github_google_aistreams//aistreams/port:status",
        "@com_github_google_aistreams//aistreams/port:statusor",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/flags:parse",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/time",
    ],
)

cc_binary(
    name = "simple_decoded_receiver_main",
    srcs = [
        "simple_decoded_receiver_main.cc",
    ],
    deps = [
        "@com_github_google_aistreams//aistreams/cc:aistreams",
        "@com_github_google_aistreams//aistreams/port:logging",
        "@com_github_google_aistreams//aistreams/port:status",
        "@com_github_google_aistreams//aistreams/port:statusor",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/flags:parse",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/time",
    ],
)
