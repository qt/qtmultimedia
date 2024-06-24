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
class QGstreamerSyncMessageFilter;
class QGstreamerBusMessageFilter;
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

    static QGstPipeline createFromFactory(const char *factory, const char *name);
    static QGstPipeline create(const char *name);

    void installMessageFilter(QGstreamerSyncMessageFilter *filter);
    void removeMessageFilter(QGstreamerSyncMessageFilter *filter);
    void installMessageFilter(QGstreamerBusMessageFilter *filter);
    void removeMessageFilter(QGstreamerBusMessageFilter *filter);

    GstStateChangeReturn setState(GstState state);

    GstPipeline *pipeline() const { return GST_PIPELINE_CAST(get()); }

    void processMessages(GstMessageType = GST_MESSAGE_ANY);

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

    void setPlaybackRate(double rate);
    double playbackRate() const;
    void applyPlaybackRate(bool instantRateChange);

    void setPosition(std::chrono::nanoseconds pos);
    std::chrono::nanoseconds position() const;
    std::chrono::milliseconds positionInMs() const;

private:
    // installs QGstPipelinePrivate as "pipeline-private" gobject property
    static QGstPipeline adopt(GstPipeline *);

    void seek(std::chrono::nanoseconds pos, double rate);
    void seek(std::chrono::nanoseconds pos);

    QGstPipelinePrivate *getPrivate() const;

    void beginConfig();
    void endConfig();
};

QT_END_NAMESPACE

#endif
