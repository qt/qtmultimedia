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

#include <QtMultimedia/qvideoframeformat.h>
#include <QtMultimedia/qvideoframe.h>
#include <QtMultimedia/private/qtmultimediaglobal_p.h>
#include <QtCore/qmutex.h>

#include <gst/video/gstvideosink.h>
#include <gst/video/video.h>

#include <QtCore/qlist.h>
#include <QtCore/qmutex.h>
#include <QtCore/qqueue.h>
#include <QtCore/qpointer.h>
#include <QtCore/qwaitcondition.h>
#include <qvideoframeformat.h>
#include <qvideoframe.h>
#include <common/qgstvideobuffer_p.h>
#include <common/qgst_p.h>

QT_BEGIN_NAMESPACE

class QGstVideoRenderer : public QObject
{
public:
    explicit QGstVideoRenderer(QGstreamerVideoSink *);
    ~QGstVideoRenderer();

    const QGstCaps &caps();

    bool start(const QGstCaps &);
    void stop();
    void unlock();
    bool proposeAllocation(GstQuery *);
    GstFlowReturn render(GstBuffer *);
    bool query(GstQuery *);
    void gstEvent(GstEvent *);

    void setActive(bool);

private:
    void updateCurrentVideoFrame(QVideoFrame);

    void notify();
    static QGstCaps createSurfaceCaps(QGstreamerVideoSink *);

    void gstEventHandleTag(GstEvent *);
    void gstEventHandleEOS(GstEvent *);

    QMutex m_sinkMutex;
    QGstreamerVideoSink *m_sink = nullptr; // written only from qt thread. so only readers on
                                           // worker threads need to acquire the lock

    // --- only accessed from gstreamer thread
    const QGstCaps m_surfaceCaps;
    QVideoFrameFormat m_format;
    GstVideoInfo m_videoInfo{};
    QGstCaps::MemoryFormat m_memoryFormat = QGstCaps::CpuMemory;
    bool m_frameMirrored = false;
    QtVideo::Rotation m_frameRotationAngle = QtVideo::Rotation::None;

    // --- only accessed from qt thread
    QVideoFrame m_currentPipelineFrame;
    QVideoFrame m_currentVideoFrame;
    bool m_isActive{ false };

    struct RenderBufferState
    {
        QGstBufferHandle buffer;
        QVideoFrameFormat format;
        QGstCaps::MemoryFormat memoryFormat;
        bool mirrored;
        QtVideo::Rotation rotationAngle;

        bool operator==(const RenderBufferState &rhs) const
        {
            return std::tie(buffer, format, memoryFormat, mirrored, rotationAngle)
                    == std::tie(rhs.buffer, rhs.format, rhs.memoryFormat, rhs.mirrored,
                                rhs.rotationAngle);
        }
    };
    RenderBufferState m_currentState;
};

class QGstVideoRendererSinkElement;

class QGstVideoRendererSink
{
public:
    GstVideoSink parent{};

    static QGstVideoRendererSinkElement createSink(QGstreamerVideoSink *surface);

private:
    static void setSink(QGstreamerVideoSink *surface);

    static GType get_type();
    static void class_init(gpointer g_class, gpointer class_data);
    static void base_init(gpointer g_class);
    static void instance_init(GTypeInstance *instance, gpointer g_class);

    static void finalize(GObject *object);

    static GstStateChangeReturn change_state(GstElement *element, GstStateChange transition);

    static GstCaps *get_caps(GstBaseSink *sink, GstCaps *filter);
    static gboolean set_caps(GstBaseSink *sink, GstCaps *caps);

    static gboolean propose_allocation(GstBaseSink *sink, GstQuery *query);

    static gboolean stop(GstBaseSink *sink);

    static gboolean unlock(GstBaseSink *sink);

    static GstFlowReturn show_frame(GstVideoSink *sink, GstBuffer *buffer);
    static gboolean query(GstBaseSink *element, GstQuery *query);
    static gboolean event(GstBaseSink *element, GstEvent *event);

    friend class QGstVideoRendererSinkElement;

    QGstVideoRenderer *renderer = nullptr;
};

class QGstVideoRendererSinkClass
{
public:
    GstVideoSinkClass parent_class;
};

class QGstVideoRendererSinkElement : public QGstBaseSink
{
public:
    using QGstBaseSink::QGstBaseSink;

    explicit QGstVideoRendererSinkElement(QGstVideoRendererSink *, RefMode);

    QGstVideoRendererSinkElement(const QGstVideoRendererSinkElement &) = default;
    QGstVideoRendererSinkElement(QGstVideoRendererSinkElement &&) noexcept = default;
    QGstVideoRendererSinkElement &operator=(const QGstVideoRendererSinkElement &) = default;
    QGstVideoRendererSinkElement &operator=(QGstVideoRendererSinkElement &&) noexcept = default;

    void setActive(bool);

    QGstVideoRendererSink *qGstVideoRendererSink() const;
};

QT_END_NAMESPACE

#endif
