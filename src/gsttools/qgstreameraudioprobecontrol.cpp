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

#include "qgstreameraudioprobecontrol_p.h"
#include <private/qgstutils_p.h>

QGstreamerAudioProbeControl::QGstreamerAudioProbeControl(QObject *parent)
    : QMediaAudioProbeControl(parent)
{
}

QGstreamerAudioProbeControl::~QGstreamerAudioProbeControl()
{
}

void QGstreamerAudioProbeControl::probeCaps(GstCaps *caps)
{
    QAudioFormat format = QGstUtils::audioFormatForCaps(caps);

    QMutexLocker locker(&m_bufferMutex);
    m_format = format;
}

bool QGstreamerAudioProbeControl::probeBuffer(GstBuffer *buffer)
{
    qint64 position = GST_BUFFER_TIMESTAMP(buffer);
    position = position >= 0
            ? position / G_GINT64_CONSTANT(1000) // microseconds
            : -1;

    QByteArray data;
#if GST_CHECK_VERSION(1,0,0)
    GstMapInfo info;
    if (gst_buffer_map(buffer, &info, GST_MAP_READ)) {
        data = QByteArray(reinterpret_cast<const char *>(info.data), info.size);
        gst_buffer_unmap(buffer, &info);
    } else {
        return true;
    }
#else
    data = QByteArray(reinterpret_cast<const char *>(buffer->data), buffer->size);
#endif

    QMutexLocker locker(&m_bufferMutex);
    if (m_format.isValid()) {
        if (!m_pendingBuffer.isValid())
            QMetaObject::invokeMethod(this, "bufferProbed", Qt::QueuedConnection);
        m_pendingBuffer = QAudioBuffer(data, m_format, position);
    }

    return true;
}

void QGstreamerAudioProbeControl::bufferProbed()
{
    QAudioBuffer audioBuffer;
    {
        QMutexLocker locker(&m_bufferMutex);
        if (!m_pendingBuffer.isValid())
            return;
        audioBuffer = m_pendingBuffer;
        m_pendingBuffer = QAudioBuffer();
    }
    emit audioBufferProbed(audioBuffer);
}
