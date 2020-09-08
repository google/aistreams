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
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef _CMLV_GRPC_SINK_H_
#define _CMLV_GRPC_SINK_H_

#include <gst/base/gstbasesink.h>

G_BEGIN_DECLS

#define CMLV_TYPE_GRPC_SINK   (cmlv_grpc_sink_get_type())
#define CMLV_GRPC_SINK(obj)   (G_TYPE_CHECK_INSTANCE_CAST((obj),CMLV_TYPE_GRPC_SINK,CmlvGrpcSink))
#define CMLV_GRPC_SINK_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST((klass),CMLV_TYPE_GRPC_SINK,CmlvGrpcSinkClass))
#define CMLV_IS_GRPC_SINK(obj)   (G_TYPE_CHECK_INSTANCE_TYPE((obj),CMLV_TYPE_GRPC_SINK))
#define CMLV_IS_GRPC_SINK_CLASS(obj)   (G_TYPE_CHECK_CLASS_TYPE((klass),CMLV_TYPE_GRPC_SINK))

typedef struct _CmlvGrpcSink CmlvGrpcSink;
typedef struct _CmlvGrpcSinkClass CmlvGrpcSinkClass;

struct _CmlvGrpcSink
{
  GstBaseSink base_grpcsink;

};

struct _CmlvGrpcSinkClass
{
  GstBaseSinkClass base_grpcsink_class;
};

GType cmlv_grpc_sink_get_type (void);

G_END_DECLS

#endif
