/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Mobility Components.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QGSTVIDEOCONNECTOR_H
#define QGSTVIDEOCONNECTOR_H

#include <gst/gst.h>

G_BEGIN_DECLS

#define GST_TYPE_VIDEO_CONNECTOR \
  (gst_video_connector_get_type())
#define GST_VIDEO_CONNECTOR(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_VIDEO_CONNECTOR, GstVideoConnector))
#define GST_VIDEO_CONNECTOR_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), GST_TYPE_VIDEO_CONNECTOR, GstVideoConnectorClass))
#define GST_IS_VIDEO_CONNECTOR(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_VIDEO_CONNECTOR))
#define GST_IS_VIDEO_CONNECTOR_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), GST_TYPE_VIDEO_CONNECTOR))

typedef struct _GstVideoConnector GstVideoConnector;
typedef struct _GstVideoConnectorClass GstVideoConnectorClass;

struct _GstVideoConnector {
  GstElement element;

  GstPad *srcpad;
  GstPad *sinkpad;

  gboolean relinked;
  gboolean failedSignalEmited;
  GstSegment segment;
  GstBuffer *latest_buffer;
};

struct _GstVideoConnectorClass {
  GstElementClass parent_class;

  /* action signal to resend new segment */
  void (*resend_new_segment) (GstElement * element, gboolean emitFailedSignal);
};

GType gst_video_connector_get_type (void);

G_END_DECLS

#endif

