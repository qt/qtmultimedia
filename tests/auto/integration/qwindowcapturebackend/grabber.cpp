// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "grabber.h"
#include "fixture.h"

#include <qtest.h>
#include <qvideoframe.h>
#include <q20vector.h>

FrameGrabber::FrameGrabber()
{
    const auto addFrame = [this](const QVideoFrame &frame) { m_frames.push_back(frame); };

    connect(this, &QVideoSink::videoFrameChanged, this, addFrame);
}

const std::vector<QVideoFrame> &FrameGrabber::getFrames() const
{
    return m_frames;
}

std::vector<QVideoFrame> FrameGrabber::waitAndTakeFrames(size_t minCount, qint64 noOlderThanTime)
{
    m_frames.clear();

    const auto enoughFramesOrStopped = [this, minCount, noOlderThanTime]() -> bool {
        if (m_stopped)
            return true; // Stop waiting

        // ensure that all signals &QVideoSink::videoFrameChanged have been processed
        QCoreApplication::processEvents(QEventLoop::AllEvents);

        if (noOlderThanTime > 0) {
            // Reject frames older than noOlderThanTime
            q20::erase_if(m_frames, [noOlderThanTime](const QVideoFrame &frame) {
                return frame.startTime() <= noOlderThanTime;
            });
        }

        return m_frames.size() >= minCount;
    };

    if (!QTest::qWaitFor(enoughFramesOrStopped, s_testTimeout))
        return {};

    if (m_stopped)
        return {};

    return std::move(m_frames);
}

bool FrameGrabber::isStopped() const
{
    return m_stopped;
}

void FrameGrabber::stop()
{
    qWarning() << "Stopping grabber";
    m_stopped = true;
}

#include "moc_grabber.cpp"
