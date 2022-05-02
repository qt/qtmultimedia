/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

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
#include <private/qgst_p.h>
#include <private/qgstpipeline_p.h>
#include <qwaitcondition.h>
#include <qmutex.h>
#include <qpointer.h>
#include <private/qgstreamervideosink_p.h>

QT_BEGIN_NAMESPACE

class QVideoSink;

class Q_MULTIMEDIA_EXPORT QGstreamerVideoOutput : public QObject
{
    Q_OBJECT

public:
    QGstreamerVideoOutput(QObject *parent = 0);
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
