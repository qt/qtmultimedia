// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qplatformvideosink_p.h"
#include "qmultimediautils_p.h"

QT_BEGIN_NAMESPACE

QPlatformVideoSink::QPlatformVideoSink(QVideoSink *parent) : QObject(parent), m_sink(parent) { }

QPlatformVideoSink::~QPlatformVideoSink() = default;

QSize QPlatformVideoSink::nativeSize() const
{
    QMutexLocker locker(&m_mutex);
    return m_nativeSize;
}

void QPlatformVideoSink::setNativeSize(QSize s)
{
    {
        QMutexLocker locker(&m_mutex);
        if (m_nativeSize == s)
            return;
        m_nativeSize = s;
    }
    emit m_sink->videoSizeChanged();
}

void QPlatformVideoSink::setVideoFrame(const QVideoFrame &frame)
{
    bool sizeChanged = false;
    QVideoFrame currentFrame;
    {
        QMutexLocker locker(&m_mutex);
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
        emit m_sink->videoSizeChanged();
    emit m_sink->videoFrameChanged(frame);
}

QVideoFrame QPlatformVideoSink::currentVideoFrame() const
{
    QMutexLocker locker(&m_mutex);
    return m_currentVideoFrame;
}

void QPlatformVideoSink::setSubtitleText(const QString &subtitleText)
{
    {
        QMutexLocker locker(&m_mutex);
        if (m_subtitleText == subtitleText)
            return;
        m_subtitleText = subtitleText;
    }
    emit m_sink->subtitleTextChanged(subtitleText);
}

QString QPlatformVideoSink::subtitleText() const
{
    QMutexLocker locker(&m_mutex);
    return m_subtitleText;
}

QT_END_NAMESPACE

#include "moc_qplatformvideosink_p.cpp"
