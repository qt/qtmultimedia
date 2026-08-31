// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "testvideosink_p.h"

#include <qsignalspy.h>
#include <qtestsystem.h>

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

void TestVideoSink::setStoreImagesEnabled(bool storeImages)
{
    if (storeImages)
        connect(this, &QVideoSink::videoFrameChanged, this, &TestVideoSink::storeImage,
                Qt::UniqueConnection);
    else
        disconnect(this, &QVideoSink::videoFrameChanged, this, &TestVideoSink::storeImage);
}

void TestVideoSink::storeImage(const QVideoFrame &frame)
{
    QImage image = frame.toImage();
    image.detach();
    m_images.push_back(std::move(image));
}

std::chrono::milliseconds TestVideoSink::durationBetweenFrames(qsizetype frameCount)
{
    Q_ASSERT(frameCount > 0);

    QElapsedTimer timer;
    qsizetype framesReceived = 0;

    QObject context;
    connect(this, &QVideoSink::videoFrameChanged, &context, [&] {
        if (framesReceived++ == 0)
            timer.start();
    });

    auto allFramesAreReceived = [&]() {
        return framesReceived > frameCount;
    };

    using namespace std::chrono;
    return QTest::qWaitFor(allFramesAreReceived, seconds(10))
            ? milliseconds(timer.elapsed() / frameCount)
            : milliseconds(0);
}

QT_END_NAMESPACE

#include "moc_testvideosink_p.cpp"
