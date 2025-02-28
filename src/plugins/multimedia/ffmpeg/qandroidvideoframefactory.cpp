// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only


#include "qandroidvideoframefactory_p.h"
#include <qvideoframe.h>

QT_BEGIN_NAMESPACE
namespace {
// This is due to a limitation of ImageReader. When limit is reached, we cannot acquire more frames
// until we close any of the previous image. That is why, When the limit is reached, we will do
// a copy of the frame data and immediately close the image.
constexpr int NATIVE_FRAME_LIMIT = 10;
}


QVideoFrame QAndroidVideoFrameFactory::createVideoFrame(QtJniTypes::Image frame,
                                                        QtVideo::Rotation rotation)
{
    const int currentCounter = m_framesCounter.fetch_add(1, std::memory_order_relaxed) + 1;

    auto frameAdapter = std::make_unique<QAndroidVideoFrameBuffer>(
            frame,
            shared_from_this(),
            currentCounter > NATIVE_FRAME_LIMIT ?
                QAndroidVideoFrameBuffer::MemoryPolicy::Copy
              : QAndroidVideoFrameBuffer::MemoryPolicy::Reuse,
            rotation);

    if (!frameAdapter->isParsed())
        return QVideoFrame{};

    const qint64 currentTimeStamp = frameAdapter->timestamp();
    QVideoFrame videoFrame(std::move(frameAdapter));

    if (m_lastTimestamp == 0)
        m_lastTimestamp = currentTimeStamp;

    videoFrame.setStartTime(m_lastTimestamp);
    videoFrame.setEndTime(currentTimeStamp);

    m_lastTimestamp = currentTimeStamp;

    return videoFrame;
}

void QAndroidVideoFrameFactory::onFrameReleased()
{
    const int currentCounter = m_framesCounter.fetch_sub(1, std::memory_order_relaxed);
    Q_ASSERT(currentCounter >= 0);
}
