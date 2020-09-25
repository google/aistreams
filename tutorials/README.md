# C++ Programming API Tutorial

This directory is a tutorial for using our C++ Programming API.

You should **not** try to build anything in this directory from within the
aistreams source tree. Instead, you should copy this entire directory to a
location outside and then build. This is intentional, as we want to demonstrate
how you can bootstrap a brand new project from scratch.

For people already familiar with Bazel, you can just pay attention to the
contents of our WORKSPACE file as it shows you what dependencies you need and
how they can be loaded. You should also pay attention to the environment
variables that are necessary to be set in order for our SDK to work as intended
(we will relax some of the environment variable requirements in a later
release).

## Building the examples

1.  Make sure that your system meets our basic
    [requirements](https://github.com/google/aistreams#sdk-system-requirements).
    This really comes down to making sure you have Gstreamer installed.

2.  Install
    [Bazel](https://docs.bazel.build/versions/3.5.0/install-ubuntu.html).

3.  Copy this directory to a location outside of the aistreams source tree; this
    is to emulate someone starting a project completely from scratch.

4.  From the root of the copied directory (the directory containing WORKSPACE),
    issue the following command to build all of the examples:

    ```shell
    bazel build -c opt //examples/...
    ```

    If all went well, you will have the binary executables under `./bazel-bin`.
    For example, try this command

    ```shell
    ./bazel-bin/examples/simple_sender_main --help
    ```

5.  Before actually running any of the binaries, make sure to set the following
    environment variables by issuing the commands from the WORKSPACE directory:

    ```shell
    export GST_PLUGIN_PATH="${PWD}/bazel-bin/external/com_github_google_aistreams/aistreams/gstreamer/gst-plugins"
    export GLOG_alsologtostderr=1
    ```

    This is needed because some of the SDK functions require aistreams' own
    Gstreamer plugins. We will remove this extra step in a future release.

## Running the examples

All of these examples require that you have at least one stream created and
ready to accept I/O. If you have not done this, make sure you are able to follow
the steps in the top level
[README.md](https://github.com/google/aistreams#google-ai-streams-sdk) up to the
point where you have a
[stream created](https://github.com/google/aistreams#creating-a-stream). This
really just amounts to making a call with our CLI **aisctl**.

If you are using the Google managed service, make sure you also set the
environment variable discussed
[here](https://github.com/google/aistreams#enabling-the-google-managed-ai-streams)
before running the binaries (these are in addition to those you've already set
earlier after the build):

```shell
export GOOGLE_APPLICATION_CREDENTIALS="/path/to/your/json/key"
```

For the time being, let's assume you have the stream "test-stream" created under
the data ingress endpoint "10.0.0.1:443" and its ssl certificate stored in the
file "ssl.crt".

### Simple sender and receiver

The first example is a pair of binaries that send and receive a protobuf
message. The sender just sends some number of greetings at a regular interval,
while the receiver simply receives them and prints the contents to screen. In a
real application, you will probably send/receive protobuf messages that are more
relevant to your use case, but you can use this as a template/starting point for
that.

The senders' source is under `examples/simple_sender_main.cc` and the receivers'
source is under `examples/simple_receiver_main.cc`. We suggest that you study
these examples to get an idea for how the API works.

You can run the binaries in any order.

To run the sender:

```shell
./bazel-bin/tutorials/simple_sender_main \
    --target_address 10.0.0.1:443 \
    --ssl_root_cert_path ssl.crt \
    --stream_name test-stream
```

To run the receiver:

```shell
./bazel-bin/tutorials/simple_receiver_main \
    --target_address 10.0.0.1:443 \
    --ssl_root_cert_path ssl.crt \
    --stream_name test-stream
```

### Simple ingester and decoded receiver

The second example is also a pair of sender and receiver. The difference is that
the sender is now a full on ingester that takes an arbitrary video source, while
the receiver directly pulls out the raw RGB image frames and prints its
dimensions to screen. Obviously, a real application would pass the RGB frame to
some other system to do more processing; e.g. run a computer vision pipeline or
a neural net. Nevertheless, you can hopefully use this as a template to
jumpstart your own application.

This example shows how you can send a video source *directly* to a stream,
without having to worry about how it's encoded or whether it is a file or a
protocol endpoint, and how you can receive RGB frames directly without ever
having to worry about codecs/protocol handling. Basically, it's videos in,
frames out; the SDK handles the conversion/transport details.

The senders' source is under `examples/simple_ingester_main.cc` and the
receivers' source is under `examples/simple_decoded_receiver_main.cc`. We
suggest that you study these examples to get an idea for how the API works.

To run the sender:

```shell
./bazel-bin/tutorials/simple_ingester_main \
    --target_address 10.0.0.1:443 \
    --ssl_root_cert_path ssl.crt \
    --stream_name test-stream \
    --source_uri my-video.mp4
```

"my-video.mp4" is a placeholder for an actual video file that you own. It can be
other video container formats as well (avi's, mkv's, etc.). It also doesn't have
to be a video file; for example, you can pass in
"rtsp://endpoint/to/your/stream" to directly send the contents of an RTSP
stream.

To run the receiver:

```shell
./bazel-bin/tutorials/simple_receiver_main \
    --target_address 10.0.0.1:443 \
    --ssl_root_cert_path ssl.crt \
    --stream_name test-stream
```
