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

#ifndef AIS_SINK_H_
#define AIS_SINK_H_

#include <gst/base/gstbasesink.h>

#include "aistreams/c/c_api.h"

G_BEGIN_DECLS

#define AIS_TYPE_SINK (ais_sink_get_type())
#define AIS_SINK(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), AIS_TYPE_SINK, AisSink))
#define AIS_SINK_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), AIS_TYPE_SINK, AisSinkClass))
#define AIS_IS_SINK(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), AIS_TYPE_SINK))
#define AIS_IS_SINK_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE((klass), AIS_TYPE_SINK))

typedef struct _AisSink AisSink;
typedef struct _AisSinkClass AisSinkClass;

/**
 * AisSink:
 * @target_address: the address to the stream server.
 *
 * The data associated with each #AisSink object.
 */
struct _AisSink {
  GstBaseSink parent;

  gchar *target_address;
  gchar *stream_name;
  gboolean authenticate_with_google;
  gboolean use_insecure_channel;
  gchar *ssl_domain_name;
  gchar *ssl_root_cert_path;
  gdouble trace_probability;

  /* An AI Streamer sender object. */
  AIS_Sender *ais_sender;

  /* An AI Streamer sender object. */
  AIS_ConnectionOptions *ais_connection_options;

  /* An AI Streamer status object. */
  AIS_Status *ais_status;
};

struct _AisSinkClass {
  GstBaseSinkClass parent_class;
};

GType ais_sink_get_type(void);

G_END_DECLS

#endif
