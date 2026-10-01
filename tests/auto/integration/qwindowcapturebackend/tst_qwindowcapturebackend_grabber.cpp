// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "tst_qwindowcapturebackend_grabber.h"

#include <QtTest/qtest.h>
#include <QtMultimedia/qvideoframe.h>
#include <QtCore/q20vector.h>
#include <QtCore/qelapsedtimer.h>
#include <QtCore/qscopeguard.h>

#include "tst_qwindowcapturebackend_fixture.h"

#include <utility>

static qint64 startTimeOf(const QVideoFrame &frame)
{
    return frame.startTime();
}

static qint64 startTimeOf(const VideoFrameInfo &info)
{
    return info.startTime;
}

static VideoFrameInfo makeVideoFrameInfo(const QVideoFrame &frame)
{
    return VideoFrameInfo{
        frame.isValid(),
        frame.size(),
        frame.pixelFormat(),
        frame.startTime(),
        frame.endTime(),
    };
}

FrameGrabber::FrameGrabber()
{
    connect(this, &QVideoSink::videoFrameChanged, this, &FrameGrabber::onFrameReceived);
}

void FrameGrabber::onFrameReceived(const QVideoFrame &frame)
{
    m_frameInfos.push_back(makeVideoFrameInfo(frame));
    if (m_retainVideoFrames)
        m_videoFrames.push_back(frame);

    if (!frame.isValid()) {
        // A null-frame ends the stream, and the next valid frame starts a new one.
        m_firstFrame.reset();
        m_streamStarted = false;
    } else if (!m_streamStarted) {
        m_firstFrame = frame;
        m_streamStarted = true;
    }
}

const std::vector<VideoFrameInfo> &FrameGrabber::getFrameInfos() const
{
    return m_frameInfos;
}

std::vector<VideoFrameInfo> FrameGrabber::waitAndTakeFrameInfos(
    size_t minCount,
    qint64 noOlderThanTime)
{
    return waitAndTake(m_frameInfos, minCount, noOlderThanTime);
}

std::vector<QVideoFrame> FrameGrabber::waitAndTakeVideoFrames(
    size_t minCount,
    qint64 noOlderThanTime)
{
    // Holding on to QVideoFrames can stall backends that use a fixed-size
    // frame pool, so only retain them while the caller is waiting for them.
    m_retainVideoFrames = true;
    const auto stopRetaining = qScopeGuard([this] {
        m_retainVideoFrames = false;
        m_videoFrames.clear();
    });

    return waitAndTake(m_videoFrames, minCount, noOlderThanTime);
}

template <typename Frame>
std::vector<Frame> FrameGrabber::waitAndTake(
    std::vector<Frame> &frames,
    size_t minCount,
    qint64 noOlderThanTime)
{
    m_frameInfos.clear();
    m_videoFrames.clear();

    const auto enoughFramesOrStopped = [this, &frames, minCount, noOlderThanTime]() -> bool {
        if (m_stopped)
            return true; // Stop waiting

        // ensure that all signals &QVideoSink::videoFrameChanged have been processed
        QCoreApplication::processEvents(QEventLoop::AllEvents);

        if (noOlderThanTime > 0) {
            // Reject frames older than noOlderThanTime
            q20::erase_if(frames, [noOlderThanTime](const Frame &frame) {
                return startTimeOf(frame) <= noOlderThanTime;
            });
        }

        return frames.size() >= minCount;
    };

    if (!QTest::qWaitFor(enoughFramesOrStopped, globalTestTimeout()))
        return {};

    if (m_stopped)
        return {};

    return std::exchange(frames, {});
}

std::optional<QVideoFrame> FrameGrabber::consumeFirstFrame()
{
    const auto firstFrameReceivedOrStopped = [this] {
        return m_stopped || m_firstFrame;
    };

    if (!QTest::qWaitFor(firstFrameReceivedOrStopped, globalTestTimeout()))
        return std::nullopt;

    if (m_stopped)
        return std::nullopt;

    return std::exchange(m_firstFrame, std::nullopt);
}

std::chrono::milliseconds FrameGrabber::durationBetweenFrames(qsizetype frameCount)
{
    Q_ASSERT(frameCount > 0);

    QElapsedTimer timer;
    qsizetype framesReceived = 0;

    QObject context;
    connect(this, &QVideoSink::videoFrameChanged, &context, [&]() {
        if (framesReceived++ == 0)
            timer.start();
    });

    auto allFramesAreReceived = [&]() {
        return framesReceived > frameCount;
    };

    using namespace std::chrono;

    // Assuming the stream runs at 1 FPS minimum. Could shorten
    // the timeout if we checked expected framerate.
    auto timeout = 2s * frameCount;
    if (isCI())
        timeout *= 5;

    return QTest::qWaitFor(allFramesAreReceived, timeout)
            ? milliseconds(timer.elapsed() / frameCount)
            : 0ms;
}

bool FrameGrabber::isStopped() const
{
    return m_stopped;
}

void FrameGrabber::onCaptureActiveChanged(bool active)
{
    // Not all backends end the stream with a null-frame, so treat activation
    // as the start of a new stream as well.
    if (active) {
        m_firstFrame.reset();
        m_streamStarted = false;
    }
}

void FrameGrabber::stop()
{
    qWarning() << "Stopping grabber";
    m_stopped = true;
}

#include "moc_tst_qwindowcapturebackend_grabber.cpp"
