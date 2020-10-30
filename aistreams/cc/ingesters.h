/*
 * Copyright 2020 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef AISTREAMS_CC_INGESTERS_H_
#define AISTREAMS_CC_INGESTERS_H_

#include <functional>

#include "absl/time/time.h"
#include "aistreams/cc/aistreams_lite.h"
#include "aistreams/port/status.h"
#include "aistreams/port/statusor.h"

namespace aistreams {

// Options to configure the ingestion.
//
// TODO: Consider using protobufs for options. You may need to bubble this up
// the chain for ConnectionOptions, SslOptions, etc.
struct IngesterOptions {
  // -------------------------------------------
  // (Required) Options to decide where to send the data.
  ConnectionOptions connection_options;
  std::string target_stream_name;

  // -------------------------------------------
  // (Optional) Options that require transcoding.

  // Options to configure the codec at which to send to the server.
  //
  // kNative: Send the data in its original codec. This happens when no local
  //          transcoding happens and is the default.
  // kH264: Send the frames in H264. This will be the default if you trigger an
  //        option below that requires a local transcode/decode.
  // kJpeg: Send the frames as Jpegs.
  // kRawRgb: Send the frames as raw RGB images.
  enum class SendCodec {
    kNative = 0,
    kH264,
    kJpeg,
    kRawRgb,
  };
  SendCodec send_codec = SendCodec::kNative;

  // Options to resize the image.
  //
  // Non-positive values mean to leave that dimension unchanged.
  // Leaving both non-positive will bypass resizing altogether.
  int resize_height = 0;
  int resize_width = 0;

  // Options to instrument packet header.
  double trace_probability = 0;
};

// Ingest the stream specified in `source_uri`, and send the data to an AI
// Streams instance specified in `options`.
//
// `options` also contains some sending/pre-processing settings, but the default
// is to pass its native codec directly to the stream server.
//
// The `source_uri` is a string that specifies your data source. It can be
// prefixed with some protocol type; leaving off the prefix assumes that it is a
// file on your local system. The following are examples:
// + rtsp://some.ipcamera.address:port.
// + file:///some/abolute/path/to/a/video/file.mp4.
// + some_relative_path_to_a_video_file.mp4.
Status Ingest(const IngesterOptions& options, absl::string_view source_uri);

}  // namespace aistreams

#endif  // AISTREAMS_CC_INGESTERS_H_
