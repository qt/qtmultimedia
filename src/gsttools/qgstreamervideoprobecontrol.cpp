/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qgstreamervideoprobecontrol_p.h"

#include "qgstutils_p.h"
#include <private/qgstvideobuffer_p.h>

QGstreamerVideoProbeControl::QGstreamerVideoProbeControl(QObject *parent)
    : QMediaVideoProbeControl(parent)
    , m_flushing(false)
    , m_frameProbed(false)
{
}

QGstreamerVideoProbeControl::~QGstreamerVideoProbeControl()
{
}

void QGstreamerVideoProbeControl::startFlushing()
{
    m_flushing = true;

    {
        QMutexLocker locker(&m_frameMutex);
        m_pendingFrame = QVideoFrame();
    }

    // only emit flush if at least one frame was probed
    if (m_frameProbed)
        emit flush();
}

void QGstreamerVideoProbeControl::stopFlushing()
{
    m_flushing = false;
}

void QGstreamerVideoProbeControl::probeCaps(GstCaps *caps)
{
#if GST_CHECK_VERSION(1,0,0)
    GstVideoInfo videoInfo;
    QVideoSurfaceFormat format = QGstUtils::formatForCaps(caps, &videoInfo);

    QMutexLocker locker(&m_frameMutex);
    m_videoInfo = videoInfo;
#else
    int bytesPerLine = 0;
    QVideoSurfaceFormat format = QGstUtils::formatForCaps(caps, &bytesPerLine);

    QMutexLocker locker(&m_frameMutex);
    m_bytesPerLine = bytesPerLine;
#endif
    m_format = format;
}

bool QGstreamerVideoProbeControl::probeBuffer(GstBuffer *buffer)
{
    QMutexLocker locker(&m_frameMutex);

    if (m_flushing || !m_format.isValid())
        return true;

    QVideoFrame frame(
#if GST_CHECK_VERSION(1,0,0)
                new QGstVideoBuffer(buffer, m_videoInfo),
#else
                new QGstVideoBuffer(buffer, m_bytesPerLine),
#endif
                m_format.frameSize(),
                m_format.pixelFormat());

    QGstUtils::setFrameTimeStamps(&frame, buffer);

    m_frameProbed = true;

    if (!m_pendingFrame.isValid())
        QMetaObject::invokeMethod(this, "frameProbed", Qt::QueuedConnection);
    m_pendingFrame = frame;

    return true;
}

void QGstreamerVideoProbeControl::frameProbed()
{
    QVideoFrame frame;
    {
        QMutexLocker locker(&m_frameMutex);
        if (!m_pendingFrame.isValid())
            return;
        frame = m_pendingFrame;
        m_pendingFrame = QVideoFrame();
    }
    emit videoFrameProbed(frame);
}
