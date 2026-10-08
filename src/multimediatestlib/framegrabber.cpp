// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "framegrabber_p.h"

#include "mediabackendutils_p.h"

#include <QtTest/qtest.h>
#include <QtTest/private/qtestblacklist_p.h>
#include <QtMultimedia/qvideoframe.h>
#include <QtCore/q20vector.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qelapsedtimer.h>
#include <QtCore/qscopeguard.h>

#include <utility>

QT_BEGIN_NAMESPACE

namespace QtMultimediaTest {

std::chrono::milliseconds surfaceCaptureTestTimeout()
{
    // A blacklisted test is expected to fail, so don't make it wait for the full
    // timeout to do so.
    if (const char *testFunction = QTest::currentTestFunction();
        testFunction && QTestPrivate::checkBlackLists(testFunction, QTest::currentDataTag())) {
        return std::chrono::seconds(1);
    }

    if (isCI())
        return std::chrono::seconds(60);
    return std::chrono::seconds(5);
}

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
        resetFirstFrame();
        m_streamStarted = false;
    } else if (!m_streamStarted) {
        m_firstFrameInfo = m_frameInfos.back();
        if (m_retainFirstVideoFrame)
            m_firstVideoFrame = frame;
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

    if (!QTest::qWaitFor(enoughFramesOrStopped, surfaceCaptureTestTimeout()))
        return {};

    if (m_stopped)
        return {};

    return std::exchange(frames, {});
}

bool FrameGrabber::waitForFirstFrame()
{
    const auto firstFrameReceivedOrStopped = [this] {
        return m_stopped || m_firstFrameInfo;
    };

    if (!QTest::qWaitFor(firstFrameReceivedOrStopped, surfaceCaptureTestTimeout()))
        return false;

    return !m_stopped;
}

std::optional<VideoFrameInfo> FrameGrabber::tryWaitForFirstFrameInfo()
{
    if (!waitForFirstFrame())
        return std::nullopt;

    return m_firstFrameInfo;
}

std::optional<QVideoFrame> FrameGrabber::consumeFirstVideoFrame()
{
    Q_ASSERT_X(
        m_retainFirstVideoFrame,
        "FrameGrabber::consumeFirstVideoFrame",
        "setRetainFirstVideoFrame() must be enabled before the stream starts");

    if (!waitForFirstFrame())
        return std::nullopt;

    return std::exchange(m_firstVideoFrame, std::nullopt);
}

void FrameGrabber::setRetainFirstVideoFrame(bool retain)
{
    m_retainFirstVideoFrame = retain;
    if (!retain)
        m_firstVideoFrame.reset();
}

void FrameGrabber::resetFirstFrame()
{
    m_firstFrameInfo.reset();
    m_firstVideoFrame.reset();
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
        resetFirstFrame();
        m_streamStarted = false;
    }
}

void FrameGrabber::stop()
{
    qWarning() << "Stopping grabber";
    m_stopped = true;
}

} // namespace QtMultimediaTest

QT_END_NAMESPACE

#include "moc_framegrabber_p.cpp"
