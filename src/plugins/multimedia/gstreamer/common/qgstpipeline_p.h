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
class QGstreamerBusMessageFilter;
struct QGstPipelinePrivate;

class QGstPipeline : public QGstBin
{
    static constexpr auto defaultQueryTimeout = std::chrono::seconds{ 5 };

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

    void installMessageFilter(QGstreamerBusMessageFilter *filter);
    void removeMessageFilter(QGstreamerBusMessageFilter *filter);

    GstStateChangeReturn setState(GstState state);

    GstPipeline *pipeline() const { return GST_PIPELINE_CAST(get()); }

    bool processNextPendingMessage(GstMessageType = GST_MESSAGE_ANY,
                                   std::chrono::nanoseconds timeout = {});
    bool processNextPendingMessage(std::chrono::nanoseconds timeout);

    void flush();

    void setPlaybackRate(double rate, bool forceFlushingSeek = false);
    double playbackRate() const;
    void applyPlaybackRate(bool forceFlushingSeek = false);

    void setPosition(std::chrono::nanoseconds pos, bool flush = true);
    std::chrono::nanoseconds position() const;
    std::chrono::milliseconds positionInMs() const;

    void setPositionAndRate(std::chrono::nanoseconds pos, double rate);

    std::optional<std::chrono::nanoseconds>
    queryPosition(std::chrono::nanoseconds timeout = defaultQueryTimeout) const;
    std::optional<std::chrono::nanoseconds>
    queryDuration(std::chrono::nanoseconds timeout = defaultQueryTimeout) const;
    std::optional<std::pair<std::chrono::nanoseconds, std::chrono::nanoseconds>>
    queryPositionAndDuration(std::chrono::nanoseconds timeout = defaultQueryTimeout) const;

    void seekToEndWithEOS();

private:
    // installs QGstPipelinePrivate as "pipeline-private" gobject property
    static QGstPipeline adopt(GstPipeline *);

    void seek(std::chrono::nanoseconds pos, double rate, bool flush);
    void seek(std::chrono::nanoseconds pos, bool flush);

    QGstPipelinePrivate *getPrivate() const;
};

QT_END_NAMESPACE

#endif
