// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsurfacecapturegrabber_p.h"

#include <qchronotimer.h>
#include <qelapsedtimer.h>
#include <qloggingcategory.h>
#include <qthread.h>
#include <qtimer.h>

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(qLcScreenCaptureGrabber, "qt.multimedia.ffmpeg.surfacecapturegrabber");

class QSurfaceCaptureGrabber::GrabbingProfiler
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

struct QSurfaceCaptureGrabber::GrabbingContext
{
    GrabbingProfiler profiler;
    QChronoTimer timer;
    QElapsedTimer elapsedTimer;
    qint64 lastFrameTime = 0;
};

class QSurfaceCaptureGrabber::GrabbingThread : public QThread
{
public:
    GrabbingThread(QSurfaceCaptureGrabber& grabber)
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
    QSurfaceCaptureGrabber& m_grabber;
};

QSurfaceCaptureGrabber::QSurfaceCaptureGrabber(ThreadPolicy threadPolicy)
{
    setFrameRate(DefaultScreenCaptureFrameRate);

    if (threadPolicy == CreateGrabbingThread)
        m_thread = std::make_unique<GrabbingThread>(*this);
}

void QSurfaceCaptureGrabber::start()
{
    if (m_thread)
        m_thread->start();
    else if (!isGrabbingContextInitialized())
        initializeGrabbingContext();
}

QSurfaceCaptureGrabber::~QSurfaceCaptureGrabber() = default;

void QSurfaceCaptureGrabber::setFrameRate(std::optional<qreal> rate)
{
    if (!rate)
        rate = DefaultScreenCaptureFrameRate;

    // Bound to a minimum
    if (*rate < MinScreenCaptureFrameRate)
        rate = MinScreenCaptureFrameRate;

    if (m_rate == *rate)
        return;

    m_rate = *rate;
    qCDebug(qLcScreenCaptureGrabber) << "Screen capture rate has been changed:" << m_rate;
}

qreal QSurfaceCaptureGrabber::frameRate() const
{
    return m_rate;
}

void QSurfaceCaptureGrabber::stop()
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

void QSurfaceCaptureGrabber::updateError(QPlatformSurfaceCapture::Error error,
                                             const QString &description)
{
    const auto prevError = std::exchange(m_prevError, error);

    if (error != QPlatformSurfaceCapture::Error::NoError
        || prevError != QPlatformSurfaceCapture::Error::NoError) {
        emit errorUpdated(error, description);
    }

    updateTimerInterval();
}

void QSurfaceCaptureGrabber::updateTimerInterval()
{
    using namespace std::chrono;

    const qreal rate = m_prevError && *m_prevError != QPlatformSurfaceCapture::Error::NoError
            ? MinScreenCaptureFrameRate
            : m_rate;
    const auto interval = round<nanoseconds>(nanoseconds(1s) / rate);

    if (m_context && m_context->timer.interval() != interval)
        m_context->timer.setInterval(interval);
}

void QSurfaceCaptureGrabber::initializeGrabbingContext()
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

            updateError(QPlatformSurfaceCapture::Error::NoError);

            emit frameGrabbed(frame);
        }
    };

    doGrab();

    m_context->timer.callOnTimeout(&m_context->timer, doGrab);
    m_context->timer.start();
}

void QSurfaceCaptureGrabber::finalizeGrabbingContext()
{
    Q_ASSERT(isGrabbingContextInitialized());
    qCDebug(qLcScreenCaptureGrabber)
            << "end screen capture thread; avg grabbing time:" << m_context->profiler.avgTime()
            << "ms, grabbings number:" << m_context->profiler.number();
    m_context.reset();
}

bool QSurfaceCaptureGrabber::isGrabbingContextInitialized() const
{
    return m_context != nullptr;
}

void QSurfaceCaptureGrabber::injectContextToGrabbingThread(QObject *context)
{
    Q_ASSERT(m_thread);
    Q_ASSERT(QThread::currentThread() == context->thread());
    context->moveToThread(m_thread.get());
}

QT_END_NAMESPACE

#include "moc_qsurfacecapturegrabber_p.cpp"
