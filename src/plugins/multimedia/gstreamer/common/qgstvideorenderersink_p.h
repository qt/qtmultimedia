// Copyright (C) 2016 Jolla Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QGSTVIDEORENDERERSINK_P_H
#define QGSTVIDEORENDERERSINK_P_H

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
#include <gst/video/gstvideosink.h>
#include <gst/video/video.h>

#include <QtCore/qlist.h>
#include <QtCore/qmutex.h>
#include <QtCore/qqueue.h>
#include <QtCore/qpointer.h>
#include <QtCore/qwaitcondition.h>
#include <qvideoframeformat.h>
#include <qvideoframe.h>
#include <qgstvideobuffer_p.h>
#include <qgst_p.h>

QT_BEGIN_NAMESPACE
class QVideoSink;

class QGstVideoRenderer : public QObject
{
    Q_OBJECT
public:
    QGstVideoRenderer(QGstreamerVideoSink *sink);
    ~QGstVideoRenderer();

    QGstCaps caps();

    bool start(const QGstCaps& caps);
    void stop();
    void unlock();
    bool proposeAllocation(GstQuery *query);

    void flush();

    GstFlowReturn render(GstBuffer *buffer);

    bool event(QEvent *event) override;
    bool query(GstQuery *query);
    void gstEvent(GstEvent *event);

private slots:
    bool handleEvent(QMutexLocker<QMutex> *locker);

private:
    void notify();
    bool waitForAsyncEvent(QMutexLocker<QMutex> *locker, QWaitCondition *condition, unsigned long time);
    void createSurfaceCaps();

    QPointer<QGstreamerVideoSink> m_sink;

    QMutex m_mutex;
    QWaitCondition m_setupCondition;
    QWaitCondition m_renderCondition;

    // --- accessed from multiple threads, need to hold mutex to access
    GstFlowReturn m_renderReturn = GST_FLOW_OK;
    bool m_active = false;

    QGstCaps m_surfaceCaps;

    QGstCaps m_startCaps;
    GstBuffer *m_renderBuffer = nullptr;

    bool m_notified = false;
    bool m_stop = false;
    bool m_flush = false;
    bool m_frameMirrored = false;
    QVideoFrame::RotationAngle m_frameRotationAngle = QVideoFrame::Rotation0;

    // --- only accessed from one thread
    QVideoFrameFormat m_format;
    GstVideoInfo m_videoInfo;
    bool m_flushed = true;
    QGstCaps::MemoryFormat memoryFormat = QGstCaps::CpuMemory;
};

class Q_MULTIMEDIA_EXPORT QGstVideoRendererSink
{
public:
    GstVideoSink parent;

    static QGstVideoRendererSink *createSink(QGstreamerVideoSink *surface);
    static void setSink(QGstreamerVideoSink *surface);

private:
    static GType get_type();
    static void class_init(gpointer g_class, gpointer class_data);
    static void base_init(gpointer g_class);
    static void instance_init(GTypeInstance *instance, gpointer g_class);

    static void finalize(GObject *object);

    static void handleShowPrerollChange(GObject *o, GParamSpec *p, gpointer d);

    static GstStateChangeReturn change_state(GstElement *element, GstStateChange transition);

    static GstCaps *get_caps(GstBaseSink *sink, GstCaps *filter);
    static gboolean set_caps(GstBaseSink *sink, GstCaps *caps);

    static gboolean propose_allocation(GstBaseSink *sink, GstQuery *query);

    static gboolean stop(GstBaseSink *sink);

    static gboolean unlock(GstBaseSink *sink);

    static GstFlowReturn show_frame(GstVideoSink *sink, GstBuffer *buffer);
    static gboolean query(GstBaseSink *element, GstQuery *query);
    static gboolean event(GstBaseSink *element, GstEvent * event);

private:
    QGstVideoRenderer *renderer = nullptr;
};


class QGstVideoRendererSinkClass
{
public:
    GstVideoSinkClass parent_class;
};

QT_END_NAMESPACE

#endif
