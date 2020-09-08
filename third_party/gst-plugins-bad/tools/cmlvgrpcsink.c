/* GStreamer
 * Copyright (C) 2019 FIXME <fixme@example.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Suite 500,
 * Boston, MA 02110-1335, USA.
 */
/**
 * SECTION:element-cmlvgrpcsink
 *
 * The grpcsink element does FIXME stuff.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch-1.0 -v fakesrc ! grpcsink ! FIXME ! fakesink
 * ]|
 * FIXME Describe what the pipeline does.
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include <gst/base/gstbasesink.h>
#include "cmlvgrpcsink.h"

GST_DEBUG_CATEGORY_STATIC (cmlv_grpc_sink_debug_category);
#define GST_CAT_DEFAULT cmlv_grpc_sink_debug_category

/* prototypes */


static void cmlv_grpc_sink_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec);
static void cmlv_grpc_sink_get_property (GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec);
static void cmlv_grpc_sink_dispose (GObject * object);
static void cmlv_grpc_sink_finalize (GObject * object);

static GstCaps *cmlv_grpc_sink_get_caps (GstBaseSink * sink, GstCaps * filter);
static gboolean cmlv_grpc_sink_set_caps (GstBaseSink * sink, GstCaps * caps);
static GstCaps *cmlv_grpc_sink_fixate (GstBaseSink * sink, GstCaps * caps);
static gboolean cmlv_grpc_sink_activate_pull (GstBaseSink * sink, gboolean active);
static void cmlv_grpc_sink_get_times (GstBaseSink * sink, GstBuffer * buffer,
    GstClockTime * start, GstClockTime * end);
static gboolean cmlv_grpc_sink_propose_allocation (GstBaseSink * sink,
    GstQuery * query);
static gboolean cmlv_grpc_sink_start (GstBaseSink * sink);
static gboolean cmlv_grpc_sink_stop (GstBaseSink * sink);
static gboolean cmlv_grpc_sink_unlock (GstBaseSink * sink);
static gboolean cmlv_grpc_sink_unlock_stop (GstBaseSink * sink);
static gboolean cmlv_grpc_sink_query (GstBaseSink * sink, GstQuery * query);
static gboolean cmlv_grpc_sink_event (GstBaseSink * sink, GstEvent * event);
static GstFlowReturn cmlv_grpc_sink_wait_event (GstBaseSink * sink,
    GstEvent * event);
static GstFlowReturn cmlv_grpc_sink_prepare (GstBaseSink * sink,
    GstBuffer * buffer);
static GstFlowReturn cmlv_grpc_sink_prepare_list (GstBaseSink * sink,
    GstBufferList * buffer_list);
static GstFlowReturn cmlv_grpc_sink_preroll (GstBaseSink * sink,
    GstBuffer * buffer);
static GstFlowReturn cmlv_grpc_sink_render (GstBaseSink * sink,
    GstBuffer * buffer);
static GstFlowReturn cmlv_grpc_sink_render_list (GstBaseSink * sink,
    GstBufferList * buffer_list);

enum
{
  PROP_0
};

/* pad templates */

static GstStaticPadTemplate cmlv_grpc_sink_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/unknown")
    );


/* class initialization */

G_DEFINE_TYPE_WITH_CODE (CmlvGrpcSink, cmlv_grpc_sink, GST_TYPE_BASE_SINK,
  GST_DEBUG_CATEGORY_INIT (cmlv_grpc_sink_debug_category, "cmlvgrpcsink", 0,
  "debug category for grpcsink element"));

static void
cmlv_grpc_sink_class_init (CmlvGrpcSinkClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstBaseSinkClass *base_sink_class = GST_BASE_SINK_CLASS (klass);

  /* Setting up pads and setting metadata should be moved to
     base_class_init if you intend to subclass this class. */
  gst_element_class_add_static_pad_template (GST_ELEMENT_CLASS(klass),
      &cmlv_grpc_sink_sink_template);

  gst_element_class_set_static_metadata (GST_ELEMENT_CLASS(klass),
      "FIXME Long name", "Generic", "FIXME Description",
      "FIXME <fixme@example.com>");

  gobject_class->set_property = cmlv_grpc_sink_set_property;
  gobject_class->get_property = cmlv_grpc_sink_get_property;
  gobject_class->dispose = cmlv_grpc_sink_dispose;
  gobject_class->finalize = cmlv_grpc_sink_finalize;
  base_sink_class->get_caps = GST_DEBUG_FUNCPTR (cmlv_grpc_sink_get_caps);
  base_sink_class->set_caps = GST_DEBUG_FUNCPTR (cmlv_grpc_sink_set_caps);
  base_sink_class->fixate = GST_DEBUG_FUNCPTR (cmlv_grpc_sink_fixate);
  base_sink_class->activate_pull = GST_DEBUG_FUNCPTR (cmlv_grpc_sink_activate_pull);
  base_sink_class->get_times = GST_DEBUG_FUNCPTR (cmlv_grpc_sink_get_times);
  base_sink_class->propose_allocation = GST_DEBUG_FUNCPTR (cmlv_grpc_sink_propose_allocation);
  base_sink_class->start = GST_DEBUG_FUNCPTR (cmlv_grpc_sink_start);
  base_sink_class->stop = GST_DEBUG_FUNCPTR (cmlv_grpc_sink_stop);
  base_sink_class->unlock = GST_DEBUG_FUNCPTR (cmlv_grpc_sink_unlock);
  base_sink_class->unlock_stop = GST_DEBUG_FUNCPTR (cmlv_grpc_sink_unlock_stop);
  base_sink_class->query = GST_DEBUG_FUNCPTR (cmlv_grpc_sink_query);
  base_sink_class->event = GST_DEBUG_FUNCPTR (cmlv_grpc_sink_event);
  base_sink_class->wait_event = GST_DEBUG_FUNCPTR (cmlv_grpc_sink_wait_event);
  base_sink_class->prepare = GST_DEBUG_FUNCPTR (cmlv_grpc_sink_prepare);
  base_sink_class->prepare_list = GST_DEBUG_FUNCPTR (cmlv_grpc_sink_prepare_list);
  base_sink_class->preroll = GST_DEBUG_FUNCPTR (cmlv_grpc_sink_preroll);
  base_sink_class->render = GST_DEBUG_FUNCPTR (cmlv_grpc_sink_render);
  base_sink_class->render_list = GST_DEBUG_FUNCPTR (cmlv_grpc_sink_render_list);

}

static void
cmlv_grpc_sink_init (CmlvGrpcSink *grpcsink)
{
}

void
cmlv_grpc_sink_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  CmlvGrpcSink *grpcsink = CMLV_GRPC_SINK (object);

  GST_DEBUG_OBJECT (grpcsink, "set_property");

  switch (property_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void
cmlv_grpc_sink_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  CmlvGrpcSink *grpcsink = CMLV_GRPC_SINK (object);

  GST_DEBUG_OBJECT (grpcsink, "get_property");

  switch (property_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void
cmlv_grpc_sink_dispose (GObject * object)
{
  CmlvGrpcSink *grpcsink = CMLV_GRPC_SINK (object);

  GST_DEBUG_OBJECT (grpcsink, "dispose");

  /* clean up as possible.  may be called multiple times */

  G_OBJECT_CLASS (cmlv_grpc_sink_parent_class)->dispose (object);
}

void
cmlv_grpc_sink_finalize (GObject * object)
{
  CmlvGrpcSink *grpcsink = CMLV_GRPC_SINK (object);

  GST_DEBUG_OBJECT (grpcsink, "finalize");

  /* clean up object here */

  G_OBJECT_CLASS (cmlv_grpc_sink_parent_class)->finalize (object);
}

static GstCaps *
cmlv_grpc_sink_get_caps (GstBaseSink * sink, GstCaps * filter)
{
  CmlvGrpcSink *grpcsink = CMLV_GRPC_SINK (sink);

  GST_DEBUG_OBJECT (grpcsink, "get_caps");

  return NULL;
}

/* notify subclass of new caps */
static gboolean
cmlv_grpc_sink_set_caps (GstBaseSink * sink, GstCaps * caps)
{
  CmlvGrpcSink *grpcsink = CMLV_GRPC_SINK (sink);

  GST_DEBUG_OBJECT (grpcsink, "set_caps");

  return TRUE;
}

/* fixate sink caps during pull-mode negotiation */
static GstCaps *
cmlv_grpc_sink_fixate (GstBaseSink * sink, GstCaps * caps)
{
  CmlvGrpcSink *grpcsink = CMLV_GRPC_SINK (sink);

  GST_DEBUG_OBJECT (grpcsink, "fixate");

  return NULL;
}

/* start or stop a pulling thread */
static gboolean
cmlv_grpc_sink_activate_pull (GstBaseSink * sink, gboolean active)
{
  CmlvGrpcSink *grpcsink = CMLV_GRPC_SINK (sink);

  GST_DEBUG_OBJECT (grpcsink, "activate_pull");

  return TRUE;
}

/* get the start and end times for syncing on this buffer */
static void
cmlv_grpc_sink_get_times (GstBaseSink * sink, GstBuffer * buffer,
    GstClockTime * start, GstClockTime * end)
{
  CmlvGrpcSink *grpcsink = CMLV_GRPC_SINK (sink);

  GST_DEBUG_OBJECT (grpcsink, "get_times");

}

/* propose allocation parameters for upstream */
static gboolean
cmlv_grpc_sink_propose_allocation (GstBaseSink * sink, GstQuery * query)
{
  CmlvGrpcSink *grpcsink = CMLV_GRPC_SINK (sink);

  GST_DEBUG_OBJECT (grpcsink, "propose_allocation");

  return TRUE;
}

/* start and stop processing, ideal for opening/closing the resource */
static gboolean
cmlv_grpc_sink_start (GstBaseSink * sink)
{
  CmlvGrpcSink *grpcsink = CMLV_GRPC_SINK (sink);

  GST_DEBUG_OBJECT (grpcsink, "start");

  return TRUE;
}

static gboolean
cmlv_grpc_sink_stop (GstBaseSink * sink)
{
  CmlvGrpcSink *grpcsink = CMLV_GRPC_SINK (sink);

  GST_DEBUG_OBJECT (grpcsink, "stop");

  return TRUE;
}

/* unlock any pending access to the resource. subclasses should unlock
 * any function ASAP. */
static gboolean
cmlv_grpc_sink_unlock (GstBaseSink * sink)
{
  CmlvGrpcSink *grpcsink = CMLV_GRPC_SINK (sink);

  GST_DEBUG_OBJECT (grpcsink, "unlock");

  return TRUE;
}

/* Clear a previously indicated unlock request not that unlocking is
 * complete. Sub-classes should clear any command queue or indicator they
 * set during unlock */
static gboolean
cmlv_grpc_sink_unlock_stop (GstBaseSink * sink)
{
  CmlvGrpcSink *grpcsink = CMLV_GRPC_SINK (sink);

  GST_DEBUG_OBJECT (grpcsink, "unlock_stop");

  return TRUE;
}

/* notify subclass of query */
static gboolean
cmlv_grpc_sink_query (GstBaseSink * sink, GstQuery * query)
{
  CmlvGrpcSink *grpcsink = CMLV_GRPC_SINK (sink);

  GST_DEBUG_OBJECT (grpcsink, "query");

  return TRUE;
}

/* notify subclass of event */
static gboolean
cmlv_grpc_sink_event (GstBaseSink * sink, GstEvent * event)
{
  CmlvGrpcSink *grpcsink = CMLV_GRPC_SINK (sink);

  GST_DEBUG_OBJECT (grpcsink, "event");

  return TRUE;
}

/* wait for eos or gap, subclasses should chain up to parent first */
static GstFlowReturn
cmlv_grpc_sink_wait_event (GstBaseSink * sink, GstEvent * event)
{
  CmlvGrpcSink *grpcsink = CMLV_GRPC_SINK (sink);

  GST_DEBUG_OBJECT (grpcsink, "wait_event");

  return GST_FLOW_OK;
}

/* notify subclass of buffer or list before doing sync */
static GstFlowReturn
cmlv_grpc_sink_prepare (GstBaseSink * sink, GstBuffer * buffer)
{
  CmlvGrpcSink *grpcsink = CMLV_GRPC_SINK (sink);

  GST_DEBUG_OBJECT (grpcsink, "prepare");

  return GST_FLOW_OK;
}

static GstFlowReturn
cmlv_grpc_sink_prepare_list (GstBaseSink * sink, GstBufferList * buffer_list)
{
  CmlvGrpcSink *grpcsink = CMLV_GRPC_SINK (sink);

  GST_DEBUG_OBJECT (grpcsink, "prepare_list");

  return GST_FLOW_OK;
}

/* notify subclass of preroll buffer or real buffer */
static GstFlowReturn
cmlv_grpc_sink_preroll (GstBaseSink * sink, GstBuffer * buffer)
{
  CmlvGrpcSink *grpcsink = CMLV_GRPC_SINK (sink);

  GST_DEBUG_OBJECT (grpcsink, "preroll");

  return GST_FLOW_OK;
}

static GstFlowReturn
cmlv_grpc_sink_render (GstBaseSink * sink, GstBuffer * buffer)
{
  CmlvGrpcSink *grpcsink = CMLV_GRPC_SINK (sink);

  GST_DEBUG_OBJECT (grpcsink, "render");

  return GST_FLOW_OK;
}

/* Render a BufferList */
static GstFlowReturn
cmlv_grpc_sink_render_list (GstBaseSink * sink, GstBufferList * buffer_list)
{
  CmlvGrpcSink *grpcsink = CMLV_GRPC_SINK (sink);

  GST_DEBUG_OBJECT (grpcsink, "render_list");

  return GST_FLOW_OK;
}

static gboolean
plugin_init (GstPlugin * plugin)
{

  /* FIXME Remember to set the rank if it's an element that is meant
     to be autoplugged by decodebin. */
  return gst_element_register (plugin, "cmlvgrpcsink", GST_RANK_NONE,
      CMLV_TYPE_GRPC_SINK);
}

/* FIXME: these are normally defined by the GStreamer build system.
   If you are creating an element to be included in gst-plugins-*,
   remove these, as they're always defined.  Otherwise, edit as
   appropriate for your external plugin package. */
#ifndef VERSION
#define VERSION "0.0.FIXME"
#endif
#ifndef PACKAGE
#define PACKAGE "FIXME_package"
#endif
#ifndef PACKAGE_NAME
#define PACKAGE_NAME "FIXME_package_name"
#endif
#ifndef GST_PACKAGE_ORIGIN
#define GST_PACKAGE_ORIGIN "http://FIXME.org/"
#endif

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    grpcsink,
    "FIXME plugin description",
    plugin_init, VERSION, "LGPL", PACKAGE_NAME, GST_PACKAGE_ORIGIN)

