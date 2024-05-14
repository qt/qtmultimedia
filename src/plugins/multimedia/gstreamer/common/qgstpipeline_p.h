// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef qgstpipeline_p_H
#define qgstpipeline_p_H

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
#include <QtCore/qobject.h>

#include "qgst_p.h"

QT_BEGIN_NAMESPACE

class QGstreamerMessage;

class QGstreamerSyncMessageFilter {
public:
    //returns true if message was processed and should be dropped, false otherwise
    virtual bool processSyncMessage(const QGstreamerMessage &message) = 0;
};


class QGstreamerBusMessageFilter {
public:
    //returns true if message was processed and should be dropped, false otherwise
    virtual bool processBusMessage(const QGstreamerMessage &message) = 0;
};

class QGstPipelinePrivate;

class QGstPipeline : public QGstBin
{
public:
    constexpr QGstPipeline() = default;
    QGstPipeline(const QGstPipeline &) = default;
    QGstPipeline(QGstPipeline &&) = default;
    QGstPipeline &operator=(const QGstPipeline &) = default;
    QGstPipeline &operator=(QGstPipeline &&) noexcept = default;
    QGstPipeline(GstPipeline *, RefMode mode);
    ~QGstPipeline();

    // installs QGstPipelinePrivate as "pipeline-private" gobject property
    static QGstPipeline create(const char *name);
    static QGstPipeline adopt(GstPipeline *);

    // This is needed to help us avoid sending QVideoFrames or audio buffers to the
    // application while we're prerolling the pipeline.
    // QMediaPlayer is still in a stopped state, while we put the gstreamer pipeline into a
    // Paused state so that we can get the required metadata of the stream and also have a fast
    // transition to play.
    bool inStoppedState() const;
    void setInStoppedState(bool stopped);

    void setFlushOnConfigChanges(bool flush);

    void installMessageFilter(QGstreamerSyncMessageFilter *filter);
    void removeMessageFilter(QGstreamerSyncMessageFilter *filter);
    void installMessageFilter(QGstreamerBusMessageFilter *filter);
    void removeMessageFilter(QGstreamerBusMessageFilter *filter);

    GstStateChangeReturn setState(GstState state);

    GstPipeline *pipeline() const { return GST_PIPELINE_CAST(get()); }

    void processMessages(GstMessageType = GST_MESSAGE_ANY);

    void dumpGraph(const char *fileName);

    template <typename Functor>
    void modifyPipelineWhileNotRunning(Functor &&fn)
    {
        beginConfig();
        fn();
        endConfig();
    }

    template <typename Functor>
    static void modifyPipelineWhileNotRunning(QGstPipeline &&pipeline, Functor &&fn)
    {
        if (pipeline)
            pipeline.modifyPipelineWhileNotRunning(fn);
        else
            fn();
    }

    void flush();

    bool seek(std::chrono::nanoseconds pos, double rate);
    bool setPlaybackRate(double rate, bool applyToPipeline = true);
    double playbackRate() const;

    bool setPosition(std::chrono::nanoseconds pos);
    std::chrono::nanoseconds position() const;
    std::chrono::milliseconds positionInMs() const;

    std::chrono::nanoseconds duration() const;
    std::chrono::milliseconds durationInMs() const;

private:
    QGstPipelinePrivate *getPrivate() const;

    void beginConfig();
    void endConfig();
};

QT_END_NAMESPACE

#endif
