/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
****************************************************************************/

#ifndef QGSTREAMERVIDEOPROBECONTROL_H
#define QGSTREAMERVIDEOPROBECONTROL_H

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

#include <private/qgsttools_global_p.h>
#include <gst/gst.h>
#include <gst/video/video.h>
#include <qmediavideoprobecontrol.h>
#include <QtCore/qmutex.h>
#include <qvideoframe.h>
#include <qvideosurfaceformat.h>

#include <private/qgstreamerbufferprobe_p.h>

QT_BEGIN_NAMESPACE

class Q_GSTTOOLS_EXPORT QGstreamerVideoProbeControl
    : public QMediaVideoProbeControl
    , public QGstreamerBufferProbe
    , public QSharedData
{
    Q_OBJECT
public:
    explicit QGstreamerVideoProbeControl(QObject *parent);
    virtual ~QGstreamerVideoProbeControl();

    void probeCaps(GstCaps *caps) override;
    bool probeBuffer(GstBuffer *buffer) override;

    void startFlushing();
    void stopFlushing();

private slots:
    void frameProbed();

private:
    QVideoSurfaceFormat m_format;
    QVideoFrame m_pendingFrame;
    QMutex m_frameMutex;
#if GST_CHECK_VERSION(1,0,0)
    GstVideoInfo m_videoInfo;
#else
    int m_bytesPerLine = 0;
#endif
    bool m_flushing = false;
    bool m_frameProbed = false; // true if at least one frame was probed
};

QT_END_NAMESPACE

#endif // QGSTREAMERVIDEOPROBECONTROL_H
