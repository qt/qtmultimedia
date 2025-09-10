// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#include <qffmpegvideosink_p.h>
#include <qffmpegvideobuffer_p.h>
#include <private/qvideoframe_p.h>

QT_BEGIN_NAMESPACE

QFFmpegVideoSink::QFFmpegVideoSink(QVideoSink *sink)
    : QPlatformVideoSink(sink)
{
}

void QFFmpegVideoSink::setRhi(QRhi *rhi)
{
    {
        QMutexLocker guard{ &m_rhiMutex };
        if (m_rhi == rhi)
            return;
        m_rhi = rhi;
    }

    emit rhiChanged();
}

void QFFmpegVideoSink::onVideoFrameChanged(const QVideoFrame &frame)
{
    QMutexLocker guard { &m_rhiMutex };
    auto *buffer = QVideoFramePrivate::hwBuffer(frame);
    if (buffer && m_rhi)
        buffer->initTextureConverter(*m_rhi);
}

QT_END_NAMESPACE

#include "moc_qffmpegvideosink_p.cpp"
