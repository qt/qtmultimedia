// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qplatformvideosink_p.h"

#include <QtMultimedia/private/qmultimediautils_p.h>

QT_BEGIN_NAMESPACE

QPlatformVideoSink::QPlatformVideoSink(QVideoSink *parent) : QObject(parent), m_sink(parent) { }

QPlatformVideoSink::~QPlatformVideoSink() = default;

QSize QPlatformVideoSink::nativeSize() const
{
    std::lock_guard locker(m_mutex);
    return m_nativeSize;
}

void QPlatformVideoSink::setNativeSize(QSize s)
{
    {
        std::lock_guard locker(m_mutex);
        if (m_nativeSize == s)
            return;
        m_nativeSize = s;
    }
    Q_EMIT m_sink->videoSizeChanged();
}

void QPlatformVideoSink::setVideoFrame(const QVideoFrame &frame)
{
    bool sizeChanged = false;
    QVideoFrame currentFrame;
    {
        std::lock_guard locker(m_mutex);
        if (frame == m_currentVideoFrame)
            return;
        m_currentVideoFrame = frame;
        m_currentVideoFrame.setSubtitleText(m_subtitleText);
        const QSize size = qRotatedFramePresentationSize(frame);
        if (size != m_nativeSize) {
            m_nativeSize = size;
            sizeChanged = true;
        }
        currentFrame = m_currentVideoFrame;
    }

    onVideoFrameChanged(currentFrame);

    // emit signals outside the mutex to avoid deadlocks on the user side
    if (sizeChanged)
        Q_EMIT m_sink->videoSizeChanged();
    Q_EMIT m_sink->videoFrameChanged(frame);
}

QVideoFrame QPlatformVideoSink::currentVideoFrame() const
{
    std::lock_guard locker(m_mutex);
    return m_currentVideoFrame;
}

void QPlatformVideoSink::setSubtitleText(const QString &subtitleText)
{
    {
        std::lock_guard locker(m_mutex);
        if (m_subtitleText == subtitleText)
            return;
        m_subtitleText = subtitleText;
    }
    Q_EMIT m_sink->subtitleTextChanged(subtitleText);
}

QString QPlatformVideoSink::subtitleText() const
{
    std::lock_guard locker(m_mutex);
    return m_subtitleText;
}

QRhiVideoSink::QRhiVideoSink(QVideoSink *sink) : QPlatformVideoSink(sink) {}

void QRhiVideoSink::setRhi(QRhi *rhi)
{
    if (rhi && !isBackendSupported(rhi->backend()))
        rhi = nullptr;
    {
        QMutexLocker guard{ &mutex() };
        if (m_rhi == rhi)
            return;
        m_rhi = rhi;
    }

    Q_EMIT rhiChanged(rhi);
}

bool QOpenGLRhiVideoSink::isBackendSupported(QRhi::Implementation backend) const
{
    return backend == QRhi::OpenGLES2;
}

QT_END_NAMESPACE

#include "moc_qplatformvideosink_p.cpp"
