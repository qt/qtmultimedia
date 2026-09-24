// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef WINDOW_CAPTURE_GRABBER_H
#define WINDOW_CAPTURE_GRABBER_H

#include <QtMultimedia/qvideoframe.h>
#include <QtMultimedia/qvideosink.h>

#include <chrono>
#include <optional>
#include <vector>

/*!
    The FrameGrabber stores frames that arrive from the window capture,
    and is used to inspect captured frames in the tests.
*/
class FrameGrabber : public QVideoSink
{
    Q_OBJECT

public:
    FrameGrabber();

    const std::vector<QVideoFrame> &getFrames() const;

    /*!
        Wait for at least \a minCount frames that are no older than noOlderThanTime.

        Returns empty if not enough frames arrived, or if grabber was stopped before global timeout
        elapsed.
    */
    std::vector<QVideoFrame> waitAndTakeFrames(size_t minCount, qint64 noOlderThanTime = 0);

    /*!
        Waits for the first frame of the current stream and consumes it, so that
        subsequent calls wait for the first frame of the next stream. A null-frame,
        or the capture becoming active, marks the start of a new stream, so the
        next valid frame after it is the first frame of that stream.

        Returns std::nullopt if no first frame arrived before the global timeout,
        or if the grabber was stopped. Can never return an invalid frame.
    */
    [[nodiscard]] std::optional<QVideoFrame> consumeFirstFrame();

    std::chrono::milliseconds durationBetweenFrames(qsizetype frameCount = 1);

    bool isStopped() const;

public slots:
    void stop();

    /*!
        Starts tracking a new stream when the capture becomes active, so that
        consumeFirstFrame() waits for the first frame of it.
    */
    void onCaptureActiveChanged(bool active);

private:
    void onFrameReceived(const QVideoFrame &frame);

    std::vector<QVideoFrame> m_frames;

    // The first frame of the current stream, until it is consumed.
    std::optional<QVideoFrame> m_firstFrame;
    // Whether we have received the first frame of the current stream.
    bool m_streamStarted = false;

    bool m_stopped = false;
};

#endif
