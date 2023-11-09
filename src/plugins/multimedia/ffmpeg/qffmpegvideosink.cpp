// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial
#include <qffmpegvideosink_p.h>
#include <qffmpegvideobuffer_p.h>

QT_BEGIN_NAMESPACE

QFFmpegVideoSink::QFFmpegVideoSink(QVideoSink *sink)
    : QPlatformVideoSink(sink)
{
}

void QFFmpegVideoSink::setRhi(QRhi *rhi)
{
    if (m_rhi == rhi)
        return;
    m_rhi = rhi;
    textureConverter = QFFmpeg::TextureConverter(rhi);
    emit rhiChanged(rhi);
}

void QFFmpegVideoSink::setVideoFrame(const QVideoFrame &frame)
{
    auto *buffer = dynamic_cast<QFFmpegVideoBuffer *>(frame.videoBuffer());
    if (buffer)
        buffer->setTextureConverter(textureConverter);

    QPlatformVideoSink::setVideoFrame(frame);
}

QT_END_NAMESPACE

#include "moc_qffmpegvideosink_p.cpp"
