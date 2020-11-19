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

/**
 * SECTION:element-aissink
 *
 * The aissink sends packets to a stream on the AI Streams server.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch-1.0 uridecodebin uri=<uri to some stream> ! aissink
 * target-address=<address to the server> stream-name=<name of your stream>
 * ]|
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/base/gstbasesink.h>
#include <gst/gst.h>
#include <stdio.h>

#include "aissink.h"

GST_DEBUG_CATEGORY_STATIC(ais_sink_debug_category);
#define GST_CAT_DEFAULT ais_sink_debug_category

/* prototypes*/

static void ais_sink_set_property(GObject *object, guint property_id,
                                  const GValue *value, GParamSpec *pspec);
static void ais_sink_get_property(GObject *object, guint property_id,
                                  GValue *value, GParamSpec *pspec);
static void ais_sink_dispose(GObject *object);
static GstCaps *ais_sink_get_caps(GstBaseSink *sink, GstCaps *filter);
static gboolean ais_sink_set_caps(GstBaseSink *sink, GstCaps *caps);
static gboolean ais_sink_start(GstBaseSink *sink);
static gboolean ais_sink_stop(GstBaseSink *sink);
static GstFlowReturn ais_sink_render(GstBaseSink *sink, GstBuffer *buffer);

/* Codes labelling the properties of the plugin.
 * The first code is a conventional zero sentinel.
 */
enum {
  PROP_0,
  PROP_TARGET_ADDRESS,
  PROP_STREAM_NAME,
  PROP_AUTHENTICATE_WITH_GOOGLE,
  PROP_USE_INSECURE_CHANNEL,
  PROP_SSL_DOMAIN_NAME,
  PROP_SSL_ROOT_CERT_PATH,
  PROP_TRACE_PROBABILITY,
};

/* pad templates */

static GstStaticPadTemplate ais_sink_sink_template = GST_STATIC_PAD_TEMPLATE(
    "sink", GST_PAD_SINK, GST_PAD_ALWAYS, GST_STATIC_CAPS_ANY);

/* class initialization */

G_DEFINE_TYPE_WITH_CODE(
    AisSink, ais_sink, GST_TYPE_BASE_SINK,
    GST_DEBUG_CATEGORY_INIT(ais_sink_debug_category, "aissink", 0,
                            "debug category for aissink element"));

static void ais_sink_class_init(AisSinkClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GstBaseSinkClass *base_sink_class = GST_BASE_SINK_CLASS(klass);

  gobject_class->set_property = ais_sink_set_property;
  gobject_class->get_property = ais_sink_get_property;

  g_object_class_install_property(
      gobject_class, PROP_TARGET_ADDRESS,
      g_param_spec_string("target-address", "Target address",
                          "Address to the AI Streams instance", NULL,
                          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(
      gobject_class, PROP_STREAM_NAME,
      g_param_spec_string("stream-name", "Stream name",
                          "Name of the destination stream", NULL,
                          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(
      gobject_class, PROP_AUTHENTICATE_WITH_GOOGLE,
      g_param_spec_boolean(
          "authenticate-with-google", "Authenticate with Google",
          "Set to true (false) when using the managed (onprem) service", FALSE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(
      gobject_class, PROP_USE_INSECURE_CHANNEL,
      g_param_spec_boolean("use-insecure-channel", "Use insecure channel",
                           "Use an insecure channel", FALSE,
                           G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(
      gobject_class, PROP_SSL_DOMAIN_NAME,
      g_param_spec_string("ssl-domain-name", "SSL domain name",
                          "The expected ssl domain name of the server", NULL,
                          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(
      gobject_class, PROP_SSL_ROOT_CERT_PATH,
      g_param_spec_string("ssl-root-cert-path", "SSL root certificate path",
                          "The file path to the root CA certificate", NULL,
                          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(
      gobject_class, PROP_TRACE_PROBABILITY,
      g_param_spec_double("trace-probability", "Trace probability",
                          "Probability to start trace for a packet", 0, 1, 0,
                          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  gst_element_class_set_static_metadata(
      GST_ELEMENT_CLASS(klass), "AI Streams sink", "Generic",
      "Send packets to AI Streams", "Google Inc");

  gst_element_class_add_static_pad_template(GST_ELEMENT_CLASS(klass),
                                            &ais_sink_sink_template);
  base_sink_class->set_caps = GST_DEBUG_FUNCPTR(ais_sink_set_caps);
  base_sink_class->get_caps = GST_DEBUG_FUNCPTR(ais_sink_get_caps);

  gobject_class->dispose = ais_sink_dispose;
  base_sink_class->start = GST_DEBUG_FUNCPTR(ais_sink_start);
  base_sink_class->stop = GST_DEBUG_FUNCPTR(ais_sink_stop);
  base_sink_class->render = GST_DEBUG_FUNCPTR(ais_sink_render);
}

/* object initialization */

static void ais_sink_init(AisSink *sink) {
  gst_base_sink_set_sync(GST_BASE_SINK(sink), TRUE);
  sink->target_address = g_strdup("");
  sink->stream_name = g_strdup("");
  sink->authenticate_with_google = FALSE;
  sink->use_insecure_channel = FALSE;
  sink->ssl_domain_name = g_strdup("aistreams.googleapis.com");
  sink->ssl_root_cert_path = g_strdup("");
  sink->trace_probability = 0;
}

/**
 * ais_sink_set_target_address
 * @sink: (not nullable): ais sink object.
 * @address: target address.
 * Returns: TRUE on success, FALSE otherwise.
 */
static gboolean ais_sink_set_target_address(AisSink *sink, const gchar *address,
                                            GError **error) {
  if (sink->ais_sender != NULL) {
    goto stream_already_open;
  }

  if (address == NULL) {
    goto null_address;
  }

  g_free(sink->target_address);
  sink->target_address = g_strdup(address);

  return TRUE;

  /* Errors */
stream_already_open : {
  g_set_error(error, GST_URI_ERROR, GST_URI_ERROR_BAD_STATE,
              "Changing the 'target_address' property when the client "
              "already connected is not supported");
  return FALSE;
}

null_address : {
  GST_ELEMENT_ERROR(sink, RESOURCE, NOT_FOUND,
                    ("A NULL target address was specified."), (NULL));
  return FALSE;
}
}

/**
 * ais_sink_set_stream_name
 * @sink: (not nullable): ais sink object.
 * @stream_name: stream name.
 * Returns: TRUE on success, FALSE otherwise.
 */
static gboolean ais_sink_set_stream_name(AisSink *sink,
                                         const gchar *stream_name,
                                         GError **error) {
  if (sink->ais_sender != NULL) {
    goto stream_already_open;
  }

  g_free(sink->stream_name);
  if (stream_name != NULL) {
    sink->stream_name = g_strdup(stream_name);
  } else {
    sink->stream_name = g_strdup("");
  }

  return TRUE;

  /* Errors */
stream_already_open : {
  g_set_error(error, GST_URI_ERROR, GST_URI_ERROR_BAD_STATE,
              "Changing the 'stream_name' property when the client "
              "already connected is not supported");
  return FALSE;
}
}

/**
 * ais_sink_set_ssl_domain_name
 * @sink: (not nullable): ais sink object.
 * @ssl_domain_name: ssl domain name.
 * Returns: TRUE on success, FALSE otherwise.
 */
static gboolean ais_sink_set_ssl_domain_name(AisSink *sink,
                                             const gchar *ssl_domain_name,
                                             GError **error) {
  if (sink->ais_sender != NULL) {
    goto stream_already_open;
  }

  g_free(sink->ssl_domain_name);
  if (ssl_domain_name != NULL) {
    sink->ssl_domain_name = g_strdup(ssl_domain_name);
  } else {
    sink->ssl_domain_name = g_strdup("");
  }

  return TRUE;

  /* Errors */
stream_already_open : {
  g_set_error(error, GST_URI_ERROR, GST_URI_ERROR_BAD_STATE,
              "Changing the 'ssl_domain_name' property when the client "
              "already connected is not supported");
  return FALSE;
}
}

/**
 * ais_sink_set_ssl_root_cert_path
 * @sink: (not nullable): ais sink object.
 * @ssl_root_cert_path: ssl domain name.
 * Returns: TRUE on success, FALSE otherwise.
 */
static gboolean ais_sink_set_ssl_root_cert_path(AisSink *sink,
                                                const gchar *ssl_root_cert_path,
                                                GError **error) {
  if (sink->ais_sender != NULL) {
    goto stream_already_open;
  }

  g_free(sink->ssl_root_cert_path);
  if (ssl_root_cert_path != NULL) {
    sink->ssl_root_cert_path = g_strdup(ssl_root_cert_path);
  } else {
    sink->ssl_root_cert_path = g_strdup("");
  }

  return TRUE;

  /* Errors */
stream_already_open : {
  g_set_error(error, GST_URI_ERROR, GST_URI_ERROR_BAD_STATE,
              "Changing the 'ssl_root_cert_path' property when the client "
              "already connected is not supported");
  return FALSE;
}
}

static void ais_sink_set_property(GObject *object, guint property_id,
                                  const GValue *value, GParamSpec *pspec) {
  AisSink *sink = AIS_SINK(object);

  switch (property_id) {
    case PROP_TARGET_ADDRESS:
      ais_sink_set_target_address(sink, g_value_get_string(value), NULL);
      break;
    case PROP_STREAM_NAME:
      ais_sink_set_stream_name(sink, g_value_get_string(value), NULL);
      break;
    case PROP_AUTHENTICATE_WITH_GOOGLE:
      sink->authenticate_with_google = g_value_get_boolean(value);
      break;
    case PROP_USE_INSECURE_CHANNEL:
      sink->use_insecure_channel = g_value_get_boolean(value);
      break;
    case PROP_SSL_DOMAIN_NAME:
      ais_sink_set_ssl_domain_name(sink, g_value_get_string(value), NULL);
      break;
    case PROP_SSL_ROOT_CERT_PATH:
      ais_sink_set_ssl_root_cert_path(sink, g_value_get_string(value), NULL);
      break;
    case PROP_TRACE_PROBABILITY:
      sink->trace_probability = g_value_get_double(value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
      break;
  }
}

static void ais_sink_get_property(GObject *object, guint property_id,
                                  GValue *value, GParamSpec *pspec) {
  AisSink *sink = AIS_SINK(object);

  switch (property_id) {
    case PROP_TARGET_ADDRESS:
      g_value_set_string(value, sink->target_address);
      break;
    case PROP_STREAM_NAME:
      g_value_set_string(value, sink->stream_name);
      break;
    case PROP_AUTHENTICATE_WITH_GOOGLE:
      g_value_set_boolean(value, sink->authenticate_with_google);
      break;
    case PROP_USE_INSECURE_CHANNEL:
      g_value_set_boolean(value, sink->use_insecure_channel);
      break;
    case PROP_SSL_DOMAIN_NAME:
      g_value_set_string(value, sink->ssl_domain_name);
      break;
    case PROP_SSL_ROOT_CERT_PATH:
      g_value_set_string(value, sink->ssl_root_cert_path);
      break;
    case PROP_TRACE_PROBABILITY:
      g_value_set_double(value, sink->trace_probability);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
      break;
  }
}

void ais_sink_dispose(GObject *object) {
  AisSink *sink = AIS_SINK(object);

  G_OBJECT_CLASS(ais_sink_parent_class)->dispose(object);

  g_free(sink->target_address);
  g_free(sink->stream_name);
  g_free(sink->ssl_domain_name);
  g_free(sink->ssl_root_cert_path);
}

static GstCaps *ais_sink_get_caps(GstBaseSink *bsink, GstCaps *filter) {
  AisSink *sink = AIS_SINK(bsink);

  GstCaps *caps = gst_pad_get_pad_template_caps(GST_BASE_SINK_PAD(sink));
  if (filter) {
    GstCaps *intersection =
        gst_caps_intersect_full(filter, caps, GST_CAPS_INTERSECT_FIRST);
    gst_caps_unref(caps);
    caps = intersection;
  }

  return caps;
}

static gboolean ais_sink_set_caps(GstBaseSink *bsink, GstCaps *caps) {
  return TRUE;
}

static gboolean ais_sink_start(GstBaseSink *bsink) {
  AisSink *sink = AIS_SINK(bsink);

  sink->ais_connection_options = AIS_NewConnectionOptions();
  AIS_SetTargetAddress(sink->target_address, sink->ais_connection_options);
  AIS_SetAuthenticateWithGoogle(sink->authenticate_with_google,
                                sink->ais_connection_options);
  AIS_SetUseInsecureChannel(sink->use_insecure_channel,
                            sink->ais_connection_options);
  AIS_SetSslDomainName(sink->ssl_domain_name, sink->ais_connection_options);
  AIS_SetSslRootCertPath(sink->ssl_root_cert_path,
                         sink->ais_connection_options);

  sink->ais_status = AIS_NewStatus();
  sink->ais_sender =
      AIS_NewSender(sink->ais_connection_options, sink->stream_name,
                    sink->trace_probability, sink->ais_status);
  if (sink->ais_sender == NULL) {
    goto failed_new_sender;
  }

  return TRUE;

failed_new_sender : {
  GST_ELEMENT_ERROR(sink, RESOURCE, NOT_FOUND,
                    ("%s", AIS_Message(sink->ais_status)),
                    ("%s", AIS_Message(sink->ais_status)));
  AIS_Log(AIS_ERROR, AIS_Message(sink->ais_status));
  AIS_Log(AIS_ERROR, "Failed to create a new sender");
  return FALSE;
}
}

static gboolean ais_sink_stop(GstBaseSink *bsink) {
  AisSink *sink = AIS_SINK(bsink);

  AIS_Packet *packet = AIS_NewEosPacket("Sender sent EOS", sink->ais_status);
  if (AIS_GetCode(sink->ais_status) != AIS_OK) {
    goto failed_eos_send;
  }
  AIS_SendPacket(sink->ais_sender, packet, sink->ais_status);
  if (AIS_GetCode(sink->ais_status) != AIS_OK) {
    goto failed_eos_send;
  }

done:
  AIS_DeletePacket(packet);
  AIS_DeleteSender(sink->ais_sender);
  AIS_DeleteStatus(sink->ais_status);
  AIS_DeleteConnectionOptions(sink->ais_connection_options);
  return TRUE;

failed_eos_send : {
  GST_ELEMENT_WARNING(sink, STREAM, FAILED,
                      ("%s", AIS_Message(sink->ais_status)),
                      ("%s", AIS_Message(sink->ais_status)));
  AIS_Log(AIS_ERROR, AIS_Message(sink->ais_status));
  AIS_Log(AIS_ERROR, "Could not send an EOS packet");
  goto done;
}
}

static GstFlowReturn ais_sink_render(GstBaseSink *bsink, GstBuffer *buffer) {
  AisSink *sink = AIS_SINK(bsink);

  AIS_Packet *packet = NULL;
  AIS_GstreamerBuffer *ais_gstreamer_buffer = AIS_NewGstreamerBuffer();

  // Set the bytes buffer.
  GstMapInfo map;
  if (gst_buffer_map(buffer, &map, GST_MAP_READ)) {
    AIS_GstreamerBufferAssign((char *)map.data, map.size, ais_gstreamer_buffer);
    gst_buffer_unmap(buffer, &map);
  } else {
    goto failed_buffer_map;
  }

  // Set the caps string.
  GstCaps *caps = gst_pad_get_current_caps(bsink->sinkpad);
  gchar *caps_string = gst_caps_to_string(caps);
  AIS_GstreamerBufferSetCapsString(caps_string, ais_gstreamer_buffer);
  g_free(caps_string);
  gst_caps_unref(caps);

  // Create the packet.
  packet = AIS_NewGstreamerBufferPacket(ais_gstreamer_buffer, sink->ais_status);
  if (packet == NULL) {
    goto failed_new_packet;
  }

  // Set/cache packet flags.
  //
  // TODO: Find a way to decide whether a buffer is the frame head.
  //       One possibility is to use GST_BUFFER_FLAG_MARKER, but this bit is
  //       not used uniformly across plugins.
  //       Turn it on uniformly for the time being as it is by far the common case.
  unsigned char is_key_frame = !GST_BUFFER_FLAG_IS_SET(buffer, GST_BUFFER_FLAG_DELTA_UNIT);
  AIS_SetIsKeyFrame(is_key_frame, packet);
  AIS_SetIsFrameHead(1, packet);

  // Send the packet.
  AIS_SendPacket(sink->ais_sender, packet, sink->ais_status);
  if (AIS_GetCode(sink->ais_status) != AIS_OK) {
    goto failed_send_packet;
  }

done:
  AIS_DeleteGstreamerBuffer(ais_gstreamer_buffer);
  AIS_DeletePacket(packet);
  return GST_FLOW_OK;

failed_buffer_map : {
  GST_ELEMENT_WARNING(sink, STREAM, FAILED,
                      ("Failed to gst_buffer_map the incoming GstBuffer"),
                      (NULL));
  goto done;
}

failed_new_packet : {
  GST_ELEMENT_WARNING(sink, STREAM, FAILED,
                      ("%s", AIS_Message(sink->ais_status)),
                      ("%s", AIS_Message(sink->ais_status)));
  goto done;
}

failed_send_packet : {
  GST_ELEMENT_WARNING(sink, STREAM, FAILED,
                      ("%s", AIS_Message(sink->ais_status)),
                      ("%s", AIS_Message(sink->ais_status)));
  AIS_Log(AIS_ERROR, AIS_Message(sink->ais_status));
  AIS_Log(AIS_ERROR,
          "Please double check the ingress endpoint and stream name you "
          "provided are valid");
  AIS_DeleteGstreamerBuffer(ais_gstreamer_buffer);
  AIS_DeletePacket(packet);
  return GST_FLOW_ERROR;
}
}

static gboolean plugin_init(GstPlugin *plugin) {
  return gst_element_register(plugin, "aissink", GST_RANK_NONE, AIS_TYPE_SINK);
}

#ifndef VERSION
#define VERSION "0.0.1"
#endif
#ifndef PACKAGE
#define PACKAGE "ais_package"
#endif
#ifndef PACKAGE_NAME
#define PACKAGE_NAME "GStreamer"
#endif
#ifndef GST_PACKAGE_ORIGIN
#define GST_PACKAGE_ORIGIN "https://gstreamer.freedesktop.org/"
#endif

GST_PLUGIN_DEFINE(GST_VERSION_MAJOR, GST_VERSION_MINOR, aissink,
                  "AI Streams Sink", plugin_init, VERSION, "Proprietary",
                  PACKAGE_NAME, GST_PACKAGE_ORIGIN)
