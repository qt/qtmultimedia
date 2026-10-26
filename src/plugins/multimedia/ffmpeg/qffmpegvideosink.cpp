// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qffmpegvideosink_p.h"

#include <private/qvideoframe_p.h>

QT_BEGIN_NAMESPACE

void QFFmpegVideoSink::onVideoFrameChanged(const QVideoFrame &frame)
{
    auto *buffer = QVideoFramePrivate::hwBuffer(frame);
    if (!buffer)
        return;

    withRhi([&](QRhi *rhi) {
        if (rhi)
            buffer->initTextureConverter(*rhi);
    });
}

QT_END_NAMESPACE

#include "moc_qffmpegvideosink_p.cpp"
