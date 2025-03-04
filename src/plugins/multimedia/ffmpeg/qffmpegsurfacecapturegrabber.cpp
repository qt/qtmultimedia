// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qffmpegsurfacecapturegrabber_p.h"

#include <qelapsedtimer.h>
#include <qloggingcategory.h>
#include <qthread.h>
#include <qtimer.h>

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(qLcScreenCaptureGrabber, "qt.multimedia.ffmpeg.surfacecapturegrabber");

class QFFmpegSurfaceCaptureGrabber::GrabbingProfiler
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

struct QFFmpegSurfaceCaptureGrabber::GrabbingContext
{
    GrabbingProfiler profiler;
    QTimer timer;
    QElapsedTimer elapsedTimer;
    qint64 lastFrameTime = 0;
};

class QFFmpegSurfaceCaptureGrabber::GrabbingThread : public QThread
{
public:
    GrabbingThread(QFFmpegSurfaceCaptureGrabber& grabber)
        : m_grabber(grabber)
    {}

protected:
    void run() override
    {
        m_grabber.initializeGrabbingContext();

        if (!m_grabber.isGrabbingContextInitialized())
            return;

        exec();
        m_grabber.finalizeGrabbingContext();
    }

private:
    QFFmpegSurfaceCaptureGrabber& m_grabber;
};

QFFmpegSurfaceCaptureGrabber::QFFmpegSurfaceCaptureGrabber(ThreadPolicy threadPolicy)
{
    setFrameRate(DefaultScreenCaptureFrameRate);

    if (threadPolicy == CreateGrabbingThread)
        m_thread = std::make_unique<GrabbingThread>(*this);
}

void QFFmpegSurfaceCaptureGrabber::start()
{
    if (m_thread)
        m_thread->start();
    else if (!isGrabbingContextInitialized())
        initializeGrabbingContext();
}

QFFmpegSurfaceCaptureGrabber::~QFFmpegSurfaceCaptureGrabber() = default;

void QFFmpegSurfaceCaptureGrabber::setFrameRate(qreal rate)
{
    rate = qBound(MinScreenCaptureFrameRate, rate, MaxScreenCaptureFrameRate);
    if (std::exchange(m_rate, rate) != rate) {
        qCDebug(qLcScreenCaptureGrabber) << "Screen capture rate has been changed:" << m_rate;

        updateTimerInterval();
    }
}

qreal QFFmpegSurfaceCaptureGrabber::frameRate() const
{
    return m_rate;
}

void QFFmpegSurfaceCaptureGrabber::stop()
{
    if (m_thread)
    {
        m_thread->quit();
        m_thread->wait();
    }
    else if (isGrabbingContextInitialized())
    {
        finalizeGrabbingContext();
    }
}

void QFFmpegSurfaceCaptureGrabber::updateError(QPlatformSurfaceCapture::Error error,
                                             const QString &description)
{
    const auto prevError = std::exchange(m_prevError, error);

    if (error != QPlatformSurfaceCapture::NoError
        || prevError != QPlatformSurfaceCapture::NoError) {
        emit errorUpdated(error, description);
    }

    updateTimerInterval();
}

void QFFmpegSurfaceCaptureGrabber::updateTimerInterval()
{
    const qreal rate = m_prevError && *m_prevError != QPlatformSurfaceCapture::NoError
            ? MinScreenCaptureFrameRate
            : m_rate;
    const int interval = static_cast<int>(1000 / rate);
    if (m_context && m_context->timer.interval() != interval)
        m_context->timer.setInterval(interval);
}

void QFFmpegSurfaceCaptureGrabber::initializeGrabbingContext()
{
    Q_ASSERT(!isGrabbingContextInitialized());
    qCDebug(qLcScreenCaptureGrabber) << "screen capture started";

    m_context = std::make_unique<GrabbingContext>();
    m_context->timer.setTimerType(Qt::PreciseTimer);
    updateTimerInterval();

    m_context->elapsedTimer.start();

    auto doGrab = [this]() {
        auto measure = m_context->profiler.measure();

        auto frame = grabFrame();

        if (frame.isValid()) {
            frame.setStartTime(m_context->lastFrameTime);
            frame.setEndTime(m_context->elapsedTimer.nsecsElapsed() / 1000);
            m_context->lastFrameTime = frame.endTime();

            updateError(QPlatformSurfaceCapture::NoError);

            emit frameGrabbed(frame);
        }
    };

    doGrab();

    m_context->timer.callOnTimeout(&m_context->timer, doGrab);
    m_context->timer.start();
}

void QFFmpegSurfaceCaptureGrabber::finalizeGrabbingContext()
{
    Q_ASSERT(isGrabbingContextInitialized());
    qCDebug(qLcScreenCaptureGrabber)
            << "end screen capture thread; avg grabbing time:" << m_context->profiler.avgTime()
            << "ms, grabbings number:" << m_context->profiler.number();
    m_context.reset();
}

bool QFFmpegSurfaceCaptureGrabber::isGrabbingContextInitialized() const
{
    return m_context != nullptr;
}

QT_END_NAMESPACE

#include "moc_qffmpegsurfacecapturegrabber_p.cpp"
