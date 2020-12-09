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

#include "aistreams/gstreamer/type_utils.h"

#include <gst/gst.h>
#include <gst/video/video.h>

#include "absl/strings/str_format.h"
#include "aistreams/base/types/gstreamer_buffer.h"
#include "aistreams/base/types/jpeg_frame.h"
#include "aistreams/base/types/raw_image.h"
#include "aistreams/base/util/packet_utils.h"
#include "aistreams/port/canonical_errors.h"
#include "aistreams/port/status.h"
#include "aistreams/port/status_macros.h"
#include "aistreams/port/statusor.h"
#include "aistreams/proto/types/raw_image.pb.h"

#define ROUND_UP_4(num) (((num) + 3) & ~0x3)

namespace aistreams {

namespace {

constexpr char kRawImageGstreamerMimeType[] = "video/x-raw";
constexpr char kJpegGstreamerMimeType[] = "image/jpeg";

// Structure that contains all metadata deducible from a GstreamerBuffer whose
// caps is of type video/x-raw.
//
// This has all the information necessary to identify the exact raw image type
// the buffer contains.
struct GstreamerRawImageInfo {
  GstVideoFormat gst_format_id;
  std::string format_name;
  int height = -1;
  int width = -1;
  int size = -1;
  int planes = -1;
  int components = -1;
  int rstride = -1;
  int pstride = -1;
};

// Parse the given gstremaer caps string (in Gstreamer's format) for the raw
// image metadata.
Status ParseAsRawImageCaps(const std::string& caps_string,
                           GstreamerRawImageInfo* info) {
  // Parse the caps.
  GstCaps* caps = gst_caps_from_string(caps_string.c_str());
  if (caps == nullptr) {
    return InvalidArgumentError(absl::StrFormat(
        "Failed to create a GstCaps from \"%s\"; make sure it is a valid "
        "cap string",
        caps_string));
  }

  // Verify this is indeed a raw image caps.
  GstStructure* structure = gst_caps_get_structure(caps, 0);
  std::string media_type(gst_structure_get_name(structure));
  if (media_type != kRawImageGstreamerMimeType) {
    gst_caps_unref(caps);
    return InvalidArgumentError(absl::StrFormat(
        "Given a GstCaps of \"%s\" which is not a raw image caps string",
        caps_string));
  }

  // Extract specific attributes and type of this raw image.
  GstVideoInfo gst_info;
  if (!gst_video_info_from_caps(&gst_info, caps)) {
    gst_caps_unref(caps);
    return InvalidArgumentError(absl::StrFormat(
        "Unable to get format information from caps %s", caps_string));
  }
  info->gst_format_id = GST_VIDEO_INFO_FORMAT(&gst_info);
  info->format_name = GST_VIDEO_INFO_NAME(&gst_info);
  info->planes = GST_VIDEO_INFO_N_PLANES(&gst_info);
  info->components = GST_VIDEO_INFO_N_COMPONENTS(&gst_info);
  info->height = GST_VIDEO_INFO_HEIGHT(&gst_info);
  info->width = GST_VIDEO_INFO_WIDTH(&gst_info);
  info->size = GST_VIDEO_INFO_SIZE(&gst_info);

  // TODO: Support just single planed images to start.
  // This is fine for most machine learning use cases.
  if (info->planes == 1) {
    info->rstride = GST_VIDEO_INFO_PLANE_STRIDE(&gst_info, 0);
    info->pstride = GST_VIDEO_INFO_COMP_PSTRIDE(&gst_info, 0);
  } else {
    if (info->planes > 1) {
      return UnimplementedError(
          "We currently support only single planed images");
    } else {
      return InvalidArgumentError("The given image has no planes");
    }
  }
  gst_caps_unref(caps);

  return OkStatus();
}

// The caller is responsible for ensuring that the GstreamerRawImageInfo is a
// RGB image. The GstreamerRawImageInfo must also be parsed from the given
// GstreamerBuffer.
StatusOr<RawImage> ToRgbRawImage(const GstreamerRawImageInfo& info,
                                 GstreamerBuffer gstreamer_buffer) {
  // Fast path when there is no extra padding.
  if (info.pstride == info.components &&
      info.rstride == info.width * info.pstride &&
      static_cast<int>(gstreamer_buffer.size()) == info.height * info.rstride) {
    RawImageDescriptor desc;
    desc.set_format(RAW_IMAGE_FORMAT_SRGB);
    desc.set_height(info.height);
    desc.set_width(info.width);
    RawImage r(desc, std::move(gstreamer_buffer).ReleaseBuffer());
    return r;
  }

  // Slow path to close extra padding.
  RawImage r(info.height, info.width, RAW_IMAGE_FORMAT_SRGB);
  for (int i = 0; i < info.height; ++i) {
    int src_row_start = info.rstride * i;
    int dst_row_start = info.width * info.components * i;
    for (int j = 0; j < info.width; ++j) {
      int src_pix_start = src_row_start + info.pstride * j;
      int dst_pix_start = dst_row_start + info.components * j;
      for (int k = 0; k < info.components; ++k) {
        r(dst_pix_start + k) = gstreamer_buffer.data()[src_pix_start + k];
      }
    }
  }
  return r;
}

StatusOr<GstreamerBuffer> GstreamerBufferPacketToGstreamerBuffer(Packet p) {
  PacketAs<GstreamerBuffer> packet_as(std::move(p));
  if (!packet_as.ok()) {
    LOG(ERROR) << packet_as.status();
    return InternalError(
        "Failed to adapt supposedly a GstreamerBuffer packet into a "
        "GstreamerBuffer");
  }
  return std::move(packet_as).ValueOrDie();
}

StatusOr<GstreamerBuffer> JpegPacketToGstreamerBuffer(Packet p) {
  PacketAs<JpegFrame> packet_as(std::move(p));
  if (!packet_as.ok()) {
    LOG(ERROR) << packet_as.status();
    return InternalError(
        "Failed to adapt supposedly a JpegFrame packet into a JpegFrame");
  }
  JpegFrame jpeg_frame = std::move(packet_as).ValueOrDie();
  GstreamerBuffer gstreamer_buffer;
  gstreamer_buffer.set_caps_string(kJpegGstreamerMimeType);
  gstreamer_buffer.assign(std::move(jpeg_frame).ReleaseBuffer());
  return gstreamer_buffer;
}

StatusOr<GstreamerBuffer> RgbRawImageToGstreamerBuffer(RawImage r) {
  // Set the caps string.
  GstreamerBuffer gstreamer_buffer;
  GstCaps* caps = gst_caps_new_simple(
      kRawImageGstreamerMimeType, "format", G_TYPE_STRING, "RGB", "width",
      G_TYPE_INT, r.width(), "height", G_TYPE_INT, r.height(), NULL);
  gchar* caps_string = gst_caps_to_string(caps);
  gstreamer_buffer.set_caps_string(caps_string);
  g_free(caps_string);
  gst_caps_unref(caps);

  // Fast path for when padding is not needed.
  // See
  // https://gstreamer.freedesktop.org/documentation/additional/design/mediatype-video-raw.html?gi-language=c
  // for more details. In this case, RGB images must pad each row up to the
  // nearest size divisible by 4.
  if (!(r.width() * r.channels() % 4)) {
    gstreamer_buffer.assign(std::move(r).ReleaseBuffer());
    return gstreamer_buffer;
  }

  // Slow path for when padding is needed.
  int row_size = r.width() * r.channels();
  int row_stride = ROUND_UP_4(row_size);
  std::string bytes;
  bytes.resize(row_stride * r.height());
  for (int i = 0; i < r.height(); ++i) {
    std::copy(r.data() + row_size * i, r.data() + row_size * (i + 1),
              const_cast<char*>(bytes.data() + row_stride * i));
  }
  gstreamer_buffer.assign(std::move(bytes));
  return gstreamer_buffer;
}

StatusOr<GstreamerBuffer> RawImagePacketToGstreamerBuffer(Packet p) {
  PacketAs<RawImage> packet_as(std::move(p));
  if (!packet_as.ok()) {
    LOG(ERROR) << packet_as.status();
    return InternalError(
        "Failed to adapt supposedly a RawImage packet into a RawImage");
  }
  RawImage raw_image = std::move(packet_as).ValueOrDie();
  return ToGstreamerBuffer(std::move(raw_image));
}

}  // namespace

StatusOr<RawImage> ToRawImage(GstreamerBuffer gstreamer_buffer) {
  GstreamerRawImageInfo info;
  auto status = ParseAsRawImageCaps(gstreamer_buffer.get_caps(), &info);
  if (!status.ok()) {
    LOG(ERROR) << status;
    return InvalidArgumentError(
        "Failed to parse the given buffer as a raw image");
  }

  switch (info.gst_format_id) {
    case GST_VIDEO_FORMAT_RGB:
      return ToRgbRawImage(info, std::move(gstreamer_buffer));
    default:
      return UnimplementedError(absl::StrFormat(
          "We currently do not support \"%s\"", info.format_name));
  }
}

StatusOr<GstreamerBuffer> ToGstreamerBuffer(Packet p) {
  PacketTypeId packet_type_id = GetPacketTypeId(p);
  switch (packet_type_id) {
    case PACKET_TYPE_GSTREAMER_BUFFER:
      return GstreamerBufferPacketToGstreamerBuffer(std::move(p));
    case PACKET_TYPE_JPEG:
      return JpegPacketToGstreamerBuffer(std::move(p));
    case PACKET_TYPE_RAW_IMAGE:
      return RawImagePacketToGstreamerBuffer(std::move(p));
    default:
      return InvalidArgumentError(
          absl::StrFormat("The given Packet has a type (%s) that cannot be "
                          "converted into a GstreamerBuffer",
                          PacketTypeId_Name(packet_type_id)));
  }
}

StatusOr<GstreamerBuffer> ToGstreamerBuffer(RawImage raw_image) {
  if (raw_image.format() != RAW_IMAGE_FORMAT_SRGB) {
    return UnimplementedError(absl::StrFormat(
        "We currently do not support raw images with your given format (%s)",
        RawImageFormat_Name(raw_image.format())));
  }
  return RgbRawImageToGstreamerBuffer(std::move(raw_image));
}

}  // namespace aistreams
