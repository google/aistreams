# This requires gstreamer to be installed on your system.
# See the gstreamer related debian packages in
# docker/install/install_debian_packages.sh.
cc_library(
    name = "gstreamer",
    srcs = glob([
        "lib/x86_64-linux-gnu/libglib-2.0.so",
        "lib/x86_64-linux-gnu/libgobject-2.0.so",
        "lib/x86_64-linux-gnu/libgstapp-1.0.so",
        "lib/x86_64-linux-gnu/libgstbase-1.0.so",
        "lib/x86_64-linux-gnu/libgstcheck-1.0.so",
        "lib/x86_64-linux-gnu/libgstcontroller-1.0.so",
        "lib/x86_64-linux-gnu/libgstnet-1.0.so",
        "lib/x86_64-linux-gnu/libgstreamer-1.0.so",
        "lib/x86_64-linux-gnu/libgstvideo-1.0.so",
        "lib/x86_64-linux-gnu/libgstrtsp-1.0.so",
        "lib/x86_64-linux-gnu/libgstrtspserver-1.0.so",
    ]),
    hdrs = glob([
        "include/glib-2.0/**/*.h",
        "include/gstreamer-1.0/**/*.h",
        "lib/x86_64-linux-gnu/glib-2.0/**/*.h",
        "lib/x86_64-linux-gnu/gstreamer-1.0/**/*.h",
    ]),
    includes = [
        "include/glib-2.0",
        "include/gstreamer-1.0",
        "lib/x86_64-linux-gnu/glib-2.0/include",
        "lib/x86_64-linux-gnu/gstreamer-1.0/include",
    ],
    linkopts = ["-pthread"],
    visibility = ["//visibility:public"],
)
