// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef FRAMEGRABBER_P_H
#define FRAMEGRABBER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qtconfigmacros.h>
#include <QtMultimedia/qvideoframe.h>
#include <QtMultimedia/qvideosink.h>

#include <chrono>
#include <optional>
#include <vector>

QT_BEGIN_NAMESPACE

namespace QtMultimediaTest {

// Generous timeout on CI, where machines are slow and heavily loaded.
[[nodiscard]] std::chrono::milliseconds surfaceCaptureTestTimeout();

struct VideoFrameInfo
{
    bool isValid = false;
    QSize size;
    QVideoFrameFormat::PixelFormat pixelFormat = QVideoFrameFormat::Format_Invalid;
    qint64 startTime = -1;
    qint64 endTime = -1;
};

/*!
    The FrameGrabber stores frames that arrive from a surface capture,
    and is used to inspect captured frames in the tests.
*/
class FrameGrabber : public QVideoSink
{
    Q_OBJECT

public:
    FrameGrabber();

    [[nodiscard]] const std::vector<VideoFrameInfo> &getFrameInfos() const;

    /*!
        Wait for at least \a minCount frames that are no older than noOlderThanTime.

        Returns empty if not enough frames arrived, or if grabber was stopped before global timeout
        elapsed.
    */
    [[nodiscard]] std::vector<VideoFrameInfo> waitAndTakeFrameInfos(
        size_t minCount,
        qint64 noOlderThanTime = 0);

    /*!
        Same as waitAndTakeFrameInfos(), but returns the video frames themselves.
        Video frames are only retained for the duration of this call.
    */
    [[nodiscard]] std::vector<QVideoFrame> waitAndTakeVideoFrames(
        size_t minCount,
        qint64 noOlderThanTime = 0);

    /*!
        Waits for the first frame of the current stream and consumes it, so that
        subsequent calls wait for the first frame of the next stream. A null-frame,
        or the capture becoming active, marks the start of a new stream, so the
        next valid frame after it is the first frame of that stream.

        Returns std::nullopt if no first frame arrived before the global timeout,
        or if the grabber was stopped. Can never return an invalid frame.
    */
    [[nodiscard]] std::optional<QVideoFrame> consumeFirstFrame();

    [[nodiscard]] std::chrono::milliseconds durationBetweenFrames(qsizetype frameCount = 1);

    [[nodiscard]] bool isStopped() const;

public slots:
    void stop();

    /*!
        Starts tracking a new stream when the capture becomes active, so that
        consumeFirstFrame() waits for the first frame of it.
    */
    void onCaptureActiveChanged(bool active);

private:
    void onFrameReceived(const QVideoFrame &frame);

    template <typename Frame>
    [[nodiscard]] std::vector<Frame> waitAndTake(
        std::vector<Frame> &frames,
        size_t minCount,
        qint64 noOlderThanTime);

    std::vector<VideoFrameInfo> m_frameInfos;

    // Only set, and m_videoFrames only populated, during waitAndTakeVideoFrames().
    bool m_retainVideoFrames = false;
    std::vector<QVideoFrame> m_videoFrames;

    // The first frame of the current stream, until it is consumed.
    std::optional<QVideoFrame> m_firstFrame;
    // Whether we have received the first frame of the current stream.
    bool m_streamStarted = false;

    bool m_stopped = false;
};

} // namespace QtMultimediaTest

QT_END_NAMESPACE

#endif // FRAMEGRABBER_P_H
