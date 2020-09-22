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

#ifndef AIS_SRC_H_
#define AIS_SRC_H_

#include <gst/base/gstpushsrc.h>
#include <gst/gst.h>

#include "aistreams/c/c_api.h"

G_BEGIN_DECLS

#define AIS_TYPE_SRC (ais_src_get_type())
#define AIS_SRC(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), AIS_TYPE_SRC, AisSrc))
#define AIS_SRC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), AIS_TYPE_SRC, AisSrcClass))
#define AIS_IS_SRC(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), AIS_TYPE_SRC))
#define AIS_IS_SRC_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), AIS_TYPE_SRC))

typedef struct _AisSrc AisSrc;
typedef struct _AisSrcClass AisSrcClass;

/**
 * AisSrc:
 *
 * The data associated with each #AisSrc object.
 */
struct _AisSrc {
  GstPushSrc element;

  /* The address (ip:port) of the remote stream server. */
  gchar *target_address;

  /* Set to true when using the google managed service; othweise false.*/
  gboolean authenticate_with_google;

  /* The name of the server stream to read from. */
  gchar *stream_name;

  /* A self identifying reciever name to the stream server. */
  gchar *receiver_name;

  /* The amount of time (in seconds) to wait for the server to deliver a packet.
   * A negative value means to wait forever.
   */
  int timeout_in_sec;

  /* Options to configure SSL information.
   *
   * use_insecure_channel: use an insecure channel to connect to the stream
   * server; set to true to enable SSL.
   *
   * ssl_domain_name: the expected ssl domain name of the server.
   *
   * ssl_root_cert_path: the path to the ssl root certificate.
   */
  gboolean use_insecure_channel;
  gchar *ssl_domain_name;
  gchar *ssl_root_cert_path;

  /* ----- private ----- */

  /* An AI Streamer status object. */
  AIS_Status *ais_status;

  /* Options to configure the AI Streamer connection. */
  AIS_ConnectionOptions *ais_connection_options;

  /* An AI Streamer receiver object. */
  AIS_Receiver *ais_receiver;
};

struct _AisSrcClass {
  GstPushSrcClass parent_class;
};

GType ais_src_get_type(void);

G_END_DECLS

#endif
