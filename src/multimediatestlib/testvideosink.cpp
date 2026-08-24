// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "testvideosink_p.h"

#include <qsignalspy.h>

QT_BEGIN_NAMESPACE

TestVideoSink::TestVideoSink(bool storeFrames) : m_storeFrames(storeFrames)
{
    connect(this, &QVideoSink::videoFrameChanged, this, &TestVideoSink::addVideoFrame);
    connect(this, &QVideoSink::videoFrameChanged, this, &TestVideoSink::videoFrameChangedSync);
}

QVideoFrame TestVideoSink::waitForFrame()
{
    QSignalSpy spy(this, &TestVideoSink::videoFrameChangedSync);
    return spy.wait() ? spy.at(0).at(0).value<QVideoFrame>() : QVideoFrame{};
}

void TestVideoSink::addVideoFrame(const QVideoFrame &frame)
{
    m_elapsedTimer.start();

    if (m_storeFrames)
        m_frameList.append(frame);

    if (frame.isValid())
        m_frameTimes.emplace_back(std::chrono::microseconds(frame.startTime()));

    ++m_totalFrames;
}

QT_END_NAMESPACE

#include "moc_testvideosink_p.cpp"
