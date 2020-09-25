# Google AI Streams SDK

The Google AI Streams is a system that hosts a collection of streams to which
I/O may be performed from the edge. The streams can be hosted on any
infrastructure supporting Kubernetes (including on-prem), or it can be fully
managed by Google in GCP on your behalf.

This repository contains the client SDK for AI Streams. It is the primary
interface for you to manage and to perform I/O against streams.

## Overview

There are two classes of stream operations: **stream management** and **stream
I/O**.

For stream management, the easiest way is to use our CLI, **aisctl**.

For stream I/O, we offer several options depending on your use case:

1.  Basic Video I/O:
    -   You may still use **aisctl** to directly ingest a video source (e.g.
        mp4, rtsp, etc.) as well as playback the ingested video from the stream
        you ingested to.
    -   We provide **Docker images** loaded with aisctl so that you can use it
        to ingest arbitrary sources visible on your infrastructure at scale.
2.  Programmatic Video I/O:
    -   You can also use our **C++ programming API** to directly ingest a source
        uri and to directly receive decoded video frames (raw RGB images) for
        your downstream processing, independent of the ingestion protocol/codec.
        The details of codec handling is done behind the scenes for you.
3.  Arbitrarily typed Packet I/O:
    -   To send/receive arbitrarily typed Packet, not necessarily video, e.g.
        any protobuf message types, JPEG, raw RGB images, etc, you can use the
        **C++ programming API**.

## Choosing an AI Streams Host

There are two options for hosting the AI Streams. The first is fully managed by
Google and the second is managed by you on-prem. We discuss both of these below.

### Enabling the Google Managed AI Streams

The Google Managed AI Streams is a fully managed solution that you can directly
enable through the GCP console. In this case, streams are hosted on GCP and
fully managed by Google. You simply create/delete clusters/streams and perform
I/O on them without having to worry about hosting or any DevOps at all.

Here are the steps to enable it:

1.  Have a GCP project ready (or create one).
2.  Have/create a service account in that project and have a JSON key
    generated/downloaded. Some instructions for this is in the official GCP
    [docs](https://cloud.google.com/docs/authentication/getting-started), but
    roughly speaking, the steps are

    -   Click on IAM & Admin, Service Accounts in the sidebar.
    -   Click on Create Service Account up top, populate the fields, and click
        Create.
    -   Click on the new service account, then click on Add Key to create a JSON
        key. At this point, the pop up box will have text that you should copy
        and paste into a file.
    -   Point the environment variable to the file you just created:

        ```shell
        export GOOGLE_APPLICATION_CREDENTIALS="/path/to/your/json/key"
        ```

3.  Enable the AI Streams API:

    -   Click on API & Services, Library in the sidebar.
    -   Type in "AI Streams" in the search box.
    -   Click on "AI Streams API" and click on "Enable".

Now you are ready to [manage streams](#stream-management) and send traffic
to/from them. But make sure you have the necessary SDK components
[installed](#sdk-system-requirements).

### Hosting AI Streams On-prem

Please contact us if you are interested in this option.

## SDK System Requirements

We have tested our SDK on Ubuntu 18.04 (and similar Linux distributions). If you
have a different bare metal setup, then we suggest that you use aisctl through
its Docker image ([discussed below](#using-aisctl-through-docker)) and use our
C++ Programming API using our base development Docker image
([also discussed below](#base-development-image)) or use it as a reference to
create your own Dockerfile.

Below are the system requirements (you can just take the default debians on
Ubuntu 18.04).

-   Python >= 3.6.9
-   Gstreamer libraries.
    -   libgstreamer1.0-0
    -   libgstreamer-plugins-base1.0-dev
    -   libgstrtspserver-1.0-0
    -   libgstrtspserver-1.0-dev
    -   gstreamer1.0-dev
    -   gstreamer1.0-plugins-base
    -   gstreamer1.0-plugins-base-apps
    -   gstreamer1.0-plugins-good
    -   gstreamer1.0-plugins-bad
    -   gstreamer1.0-plugins-ugly
    -   gstreamer1.0-libav

You only need the -dev Debians if you plan to build our library / use the C++
programming API. Feel free to take a look at our
[Dockerfiles](https://github.com/google/aistreams/tree/master/docker) to steps
that you can follow.

### Downloading and Installing Pre-Built Binaries

You can download aisctl from our
[releases page](https://github.com/google/aistreams/releases). It is a python3
pip wheel that you can install like so:

```shell
pip install aistreams-<version>-py3-none-any.whl
```

Even though it does not mark any platform in its suffix, it is in fact dependent
on the system requirements delineated earlier. These details will be properly
taken care of in a later release.

Once you've done this, you should be able to access aisctl:

```shell
aisctl --help
```

### Docker images

Below are the docker images that you can use when you are on a different bare
metal environment or simply do not want to pollute your system. The only
prerequisite here is that you have Docker installed on your system.

You may also use these images in your own Kubernetes cluster to scale the number
of camera ingestions or use them to form a base layer for your own binary.

#### Using aisctl through Docker

You can use aisctl through a Docker image that runs it as an ENTRYPOINT. You can
download the image with the following command

```shell
docker pull gcr.io/aistreams-public-images/aistreams-sdk-cli
```

After you've done that, you can run it as if you had installed it on your system
like so:

```shell
docker run gcr.io/aistreams-public-images/aistreams-sdk-cli --help
```

It may be helpful to make a few shell aliases and/or tag the image to a
different name so that your user experience feels as if aisctl was natively
installed.

When you use the docker container in this way, you also need to make sure it is
able to access your host's network and any local files; e.g. you may need to use
pass in network=host or explicitly mount a directory that contains your files of
interest.

#### Base development/runtime image

You may download our development image like so:

```shell
docker pull gcr.io/aistreams-public-images/aistreams-sdk-dev
```

or the runtime image like so: `shell docker pull
gcr.io/aistreams-public-images/aistreams-sdk-runtime`

You may use these as base layer in composing your `Dockerfile` or use it as a
development environment.

Note that we do not have a copy of our source code inside the development image;
if you need it, then you can explicitly `git clone` it from within.

## Stream Management

Stream management refers to the class of operations to create, delete, and list
your stream resources ("CRUD"). Another way of thinking about them is
construction/destruction/lifetime management of your resources.

The instructions here all use aisctl. When in doubt, pass --help to aisctl for
more details.

### Managing clusters

This is necessary only for the Google managed service. If you are on-prem you
can skip to [managing streams](#managing-streams).

#### Creating a cluster

For the managed service, you must first create a cluster, into which you may add
streams. To create a cluster, you can run the following

```shell
aisctl managed cluster --project-id my-project create --cluster-name my-cluster
```

This command will create a new cluster "my-cluster" in your GCP project
"my-project" (the project-id is the GCP project ID which is something already
assigned when your project is created).

Note that this command may take some time to complete. This is generally the
case for resource creation/deletion on GCP. (The programming API will offer an
async option, but the CLI is blocking).

#### Listing clusters

```shell
aisctl managed cluster --project-id my-project list
```

This command will list all of your existing clusters along with the details
necessary to connect to them later in [managing streams](#managing-streams). In
particular, it will print the following for each available cluster:

-   The address to the data ingress. This is the ip:port to the data ingress, to
    which you will direct your stream I/O operations to.
-   The ssl certificate to the data ingress. This is the ssl certificate to the
    data ingress, which you should copy/save to a file. It is required to be
    presented when you do stream I/O.

#### Deleting a cluster

```shell
aisctl managed cluster --project-id my-project create --cluster-name my-cluster
```

This command will delete a cluster "my-cluster" in your GCP project "my-project"
(the project-id is the GCP project ID which is something already assigned when
your project is created).

Generally, you only do this when you're done with all you need to do with the
streams and want to cleanup resources.

### Managing streams

Before you can send/receive data to streams, you must... well, create them. You
should definitely delete them when you're done too so you do not waste resources
or money.

We will go over how you can manage streams using aisctl, and specifically for
the Google managed service. We will go over how to do this on-prem in the
[last subsection](#managing-streams-on-prem).

#### Creating a stream

```shell
aisctl managed stream --project-id my-project --cluster-name my-cluster \
    create -s my-stream
```

This command will create a new stream "my-stream" in your cluster "my-cluster".

#### Listing streams

```shell
aisctl managed stream --project-id my-project --cluster-name my-cluster list
```

This command will list all streams resident in your cluster "my-cluster".

#### Deleting a stream

```shell
aisctl managed stream --project-id my-project --cluster-name my-cluster \
    delete -s my-stream
```

This command will delete the stream "my-stream" in your cluster "my-cluster".

#### Managing streams on-prem

To manage streams on prem, you would use aisctl, but just some subcommands and
flags are changed. We list the commands for each operation for your reference
and discuss what the substitutions mean.

To create a stream:

```shell
aisctl onprem stream -t <ingress-endpoint> \
                     --ssl-root-cert-path <path-to-the-crt> \
                     create -s my-stream
```

To list streams:

```shell
aisctl onprem stream -t <ingress-endpoint> \
                     --ssl-root-cert-path <path-to-the-crt> \
                     list
```

To delete a stream:

```shell
aisctl onprem stream -t <ingress-endpoint> \
                     --ssl-root-cert-path <path-to-the-crt> \
                     delete -s my-stream
```

The differences of these compared to the Google managed service is just that you
pass "onprem" instead of "managed", and you directly pass in the data ingress'
endpoint and the ssl certificate.

## Stream I/O

We will go over how you can ingest/playback a video source using aisctl. For
other use patterns that require the C++ Programming API, we refer you to the
code samples under the
[tutorials](https://github.com/google/aistreams/tree/master/tutorials)
directory.

### Ingesting a video source

To ingest a video file into a stream:

```shell
aisctl ingest -t <ingress-endpoint> \
              --ssl-root-cert-path=<path-to-ssl-crt> \
              -a \
              -s my-stream \
              -i my-video.mp4
```

This command will read my-video.mp4 and send its frames (encoded) to the stream
"my-stream". The ingress endpoint and path of the ssl crt were given when you
listed the clusters (if you're using the Google managed service), or is directly
given to you by your sysadmin or a custom mechanism if you are on prem.

The -a flag should be passed if you are ingesting to a stream on the Google
managed service (the long option is --authenticate-with-google). For on-prem,
this should be omitted.

You can ingest other forms of video sources; for example, if you wan to ingest
an rtsp endpoint, just pass `-i rtsp://url/of/your/camera/endpoint` instead of
what's listed above.

### Playback a stream

After you ingested a video source, you can play it back like so:

```shell
aisctl playback -t <ingress-endpoint> \
                --ssl-root-cert-path=<path-to-ssl-crt> \
                -a \
                -s my-stream
```

Obviously, you need to have a monitor attached to see anything. In general, you
can playback any stream whose Packets are convertible to raw images; i.e. they
need not originate from the `aistcl ingest`.

Again `-a` is required for the Google Managed Service and should be omitted for
on-prem.
