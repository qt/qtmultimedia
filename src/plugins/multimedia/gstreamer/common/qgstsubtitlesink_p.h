// Copyright (C) 2021 The Qt Company
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QGSTSUBTITLESINK_P_H
#define QGSTSUBTITLESINK_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtMultimedia/private/qtmultimediaglobal_p.h>

#include <QtCore/qlist.h>
#include <QtCore/qmutex.h>
#include <QtCore/qqueue.h>
#include <QtCore/qpointer.h>
#include <QtCore/qwaitcondition.h>
#include <qgst_p.h>
#include <gst/base/gstbasesink.h>

QT_BEGIN_NAMESPACE

class QGstreamerVideoSink;

class Q_MULTIMEDIA_EXPORT QGstSubtitleSink
{
public:
    GstBaseSink parent;

    static QGstSubtitleSink *createSink(QGstreamerVideoSink *sink);

private:
    static GType get_type();
    static void class_init(gpointer g_class, gpointer class_data);
    static void base_init(gpointer g_class);
    static void instance_init(GTypeInstance *instance, gpointer g_class);

    static void finalize(GObject *object);

    static GstStateChangeReturn change_state(GstElement *element, GstStateChange transition);

    static GstCaps *get_caps(GstBaseSink *sink, GstCaps *filter);
    static gboolean set_caps(GstBaseSink *sink, GstCaps *caps);

    static gboolean propose_allocation(GstBaseSink *sink, GstQuery *query);

    static GstFlowReturn wait_event(GstBaseSink * sink, GstEvent * event);
    static GstFlowReturn render(GstBaseSink *sink, GstBuffer *buffer);

private:
    QGstreamerVideoSink *sink = nullptr;
};


class QGstSubtitleSinkClass
{
public:
    GstBaseSinkClass parent_class;
};

QT_END_NAMESPACE

#endif
