// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "videosingleton.h"

VideoSingleton::VideoSingleton(QObject * parent) : QObject(parent)
{ }

QUrl VideoSingleton::source1() const
{
    return m_source1;
}
void VideoSingleton::setSource1(const QUrl &source1)
{
    if (source1 == m_source1)
        return;
    m_source1 = source1;
    emit source1Changed();
}
QUrl VideoSingleton::source2() const
{
    return m_source2;
}
void VideoSingleton::setSource2(const QUrl &source2)
{
    if (source2 == m_source2)
        return;
    m_source2 = source2;
    emit source2Changed();
}
qreal VideoSingleton::volume() const
{
    return m_volume;
}
void VideoSingleton::setVolume(qreal volume)
{
    if (volume == m_volume)
        return;
    m_volume = volume;
    emit volumeChanged();
}
QUrl VideoSingleton::videoPath() const
{
    return m_videoPath;
}
void VideoSingleton::setVideoPath(const QUrl &videoPath)
{
    if (m_videoPath == videoPath)
        return;
    m_videoPath = videoPath;
    emit videoPathChanged();
}

#include "moc_videosingleton.cpp"
