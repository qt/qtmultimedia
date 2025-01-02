// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qffmpegsurfacecapturethread_p.h"

#include <qtimer.h>
#include <qloggingcategory.h>

QT_BEGIN_NAMESPACE

static Q_LOGGING_CATEGORY(qLcScreenCaptureThread, "qt.multimedia.ffmpeg.surfacecapturethread");

namespace {

class GrabbingProfiler
{
public:
    auto measure()
    {
        m_elapsedTimer.start();
        return qScopeGuard([&]() {
            const auto nsecsElapsed = m_elapsedTimer.nsecsElapsed();
            ++m_number;
            m_wholeTime += nsecsElapsed;

#ifdef DUMP_SCREEN_CAPTURE_PROFILING
            qDebug() << "screen grabbing time:" << nsecsElapsed << "avg:" << avgTime()
                     << "number:" << m_number;
#endif
        });
    }

    qreal avgTime() const
    {
        return m_number ? m_wholeTime / (m_number * 1000000.) : 0.;
    }

    qint64 number() const
    {
        return m_number;
    }

private:
    QElapsedTimer m_elapsedTimer;
    qint64 m_wholeTime = 0;
    qint64 m_number = 0;
};

} // namespace

QFFmpegSurfaceCaptureThread::QFFmpegSurfaceCaptureThread()
{
    setFrameRate(DefaultScreenCaptureFrameRate);
}

QFFmpegSurfaceCaptureThread::~QFFmpegSurfaceCaptureThread() = default;

void QFFmpegSurfaceCaptureThread::setFrameRate(qreal rate)
{
    rate = qBound(MinScreenCaptureFrameRate, rate, MaxScreenCaptureFrameRate);
    if (std::exchange(m_rate, rate) != rate) {
        qCDebug(qLcScreenCaptureThread) << "Screen capture rate has been changed:" << m_rate;

        updateTimerInterval();
    }
}

qreal QFFmpegSurfaceCaptureThread::frameRate() const
{
    return m_rate;
}

void QFFmpegSurfaceCaptureThread::stop()
{
    quit();
    wait();
}

void QFFmpegSurfaceCaptureThread::run()
{
    qCDebug(qLcScreenCaptureThread) << "start screen capture thread";

    m_timer = std::make_unique<QTimer>();
    // should be deleted in this thread
    auto deleter = qScopeGuard([this]() { m_timer.reset(); });
    m_timer->setTimerType(Qt::PreciseTimer);
    updateTimerInterval();

    QElapsedTimer elapsedTimer;
    elapsedTimer.start();

    qint64 lastFrameTime = 0;

    GrabbingProfiler profiler;

    auto doGrab = [&]() {
        auto measure = profiler.measure();

        auto frame = grabFrame();

        if (frame.isValid()) {
            frame.setStartTime(lastFrameTime);
            frame.setEndTime(elapsedTimer.nsecsElapsed() / 1000);
            lastFrameTime = frame.endTime();

            updateError(QPlatformSurfaceCapture::NoError);

            emit frameGrabbed(frame);
        }
    };

    doGrab();

    m_timer->callOnTimeout(m_timer.get(), doGrab);
    m_timer->start();

    exec();

    qCDebug(qLcScreenCaptureThread)
            << "end screen capture thread; avg grabbing time:" << profiler.avgTime()
            << "ms, grabbings number:" << profiler.number();
}

void QFFmpegSurfaceCaptureThread::updateError(QPlatformSurfaceCapture::Error error,
                                             const QString &description)
{
    const auto prevError = std::exchange(m_prevError, error);

    if (error != QPlatformSurfaceCapture::NoError
        || prevError != QPlatformSurfaceCapture::NoError) {
        emit errorUpdated(error, description);
    }

    updateTimerInterval();
}

void QFFmpegSurfaceCaptureThread::updateTimerInterval()
{
    const qreal rate = m_prevError && *m_prevError != QPlatformSurfaceCapture::NoError
            ? MinScreenCaptureFrameRate
            : m_rate;
    const int interval = static_cast<int>(1000 / rate);
    if (m_timer && m_timer->interval() != interval)
        m_timer->setInterval(interval);
}

QT_END_NAMESPACE

#include "moc_qffmpegsurfacecapturethread_p.cpp"
