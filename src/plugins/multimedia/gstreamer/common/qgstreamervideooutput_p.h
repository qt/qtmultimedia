// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QGSTREAMERVIDEOOUTPUT_P_H
#define QGSTREAMERVIDEOOUTPUT_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qobject.h>
#include <private/qtmultimediaglobal_p.h>
#include <private/qmultimediautils_p.h>
#include <qgst_p.h>
#include <qgstpipeline_p.h>
#include <qwaitcondition.h>
#include <qmutex.h>
#include <qpointer.h>
#include <qgstreamervideosink_p.h>

QT_BEGIN_NAMESPACE

class QVideoSink;

class Q_MULTIMEDIA_EXPORT QGstreamerVideoOutput : public QObject
{
    Q_OBJECT

public:
    static QMaybe<QGstreamerVideoOutput *> create(QObject *parent = nullptr);
    ~QGstreamerVideoOutput();

    void setVideoSink(QVideoSink *sink);
    QGstreamerVideoSink *gstreamerVideoSink() const { return m_videoSink; }

    void setPipeline(const QGstPipeline &pipeline);

    QGstElement gstElement() const { return gstVideoOutput; }
    void linkSubtitleStream(QGstElement subtitleSrc);
    void unlinkSubtitleStream();

    void setIsPreview();
    void flushSubtitles();

private:
    QGstreamerVideoOutput(QGstElement videoConvert, QGstElement videoSink, QObject *parent);

    void doLinkSubtitleStream();

    QPointer<QGstreamerVideoSink> m_videoSink;
    bool isFakeSink = true;

    // Gst elements
    QGstPipeline gstPipeline;

    QGstBin gstVideoOutput;
    QGstElement videoQueue;
    QGstElement videoConvert;
    QGstElement videoSink;

    QGstElement subtitleSrc;
    QGstElement subtitleSink;
};

QT_END_NAMESPACE

#endif
