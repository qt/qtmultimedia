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
