// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef FAKE_H
#define FAKE_H

#include <qvideosink.h>
#include <qvideoframe.h>
#include <qelapsedtimer.h>
#include <qsignalspy.h>

QT_USE_NAMESPACE

/*
    This is a simple video surface which records all presented frames.
*/
class TestVideoSink : public QVideoSink
{
    Q_OBJECT
public:
    explicit TestVideoSink(bool storeFrames = true) : m_storeFrames(storeFrames)
    {
        connect(this, &QVideoSink::videoFrameChanged, this, &TestVideoSink::addVideoFrame);
        connect(this, &QVideoSink::videoFrameChanged, this, &TestVideoSink::videoFrameChangedSync);
    }

    QVideoFrame waitForFrame()
    {
        QSignalSpy spy(this, &TestVideoSink::videoFrameChangedSync);
        return spy.wait() ? spy.at(0).at(0).value<QVideoFrame>() : QVideoFrame{};
    }

private Q_SLOTS:
    void addVideoFrame(const QVideoFrame &frame)
    {
        if (!m_elapsedTimer.isValid())
            m_elapsedTimer.start();
        else
            m_elapsedTimer.restart();

        if (m_storeFrames)
            m_frameList.append(frame);
        ++m_totalFrames;
    }

signals:
    void videoFrameChangedSync(const QVideoFrame &frame);

public:
    QList<QVideoFrame> m_frameList;
    int m_totalFrames = 0; // used instead of the list when frames are not stored
    QElapsedTimer m_elapsedTimer;

private:
    bool m_storeFrames;
};

class TestVideoOutput : public QObject
{
    Q_OBJECT
public:
    TestVideoOutput() = default;

    Q_INVOKABLE QVideoSink *videoSink() { return &m_sink; }

    TestVideoSink m_sink;
};

static void setVideoSinkAsyncFramesCounter(QVideoSink &sink, std::atomic_int &counter)
{
    QObject::connect(
            &sink, &QVideoSink::videoFrameChanged, &sink, [&counter]() { ++counter; },
            Qt::DirectConnection);
}

#endif // FAKE_H
