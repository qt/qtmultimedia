// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef FAKE_H
#define FAKE_H

#include <private/testvideosink_p.h>

QT_USE_NAMESPACE

class TestVideoOutput : public QObject
{
    Q_OBJECT
public:
    TestVideoOutput() = default;

    Q_INVOKABLE QVideoSink *videoSink() { return &m_sink; }

    TestVideoSink m_sink;
};

inline void setVideoSinkAsyncFramesCounter(QVideoSink &sink, std::atomic_int &counter)
{
    QObject::connect(
            &sink, &QVideoSink::videoFrameChanged, &sink, [&counter]() { ++counter; },
            Qt::DirectConnection);
}

#endif // FAKE_H
