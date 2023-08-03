// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef WINDOW_CAPTURE_GRABBER_H
#define WINDOW_CAPTURE_GRABBER_H

#include <qvideosink.h>
#include <vector>

QT_USE_NAMESPACE

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

    bool isStopped() const;

public slots:
    void stop();

private:
    std::vector<QVideoFrame> m_frames;
    bool m_stopped = false;
};

#endif
