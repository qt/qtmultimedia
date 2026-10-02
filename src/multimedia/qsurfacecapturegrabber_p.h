// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSURFACECAPTUREGRABBER_P_H
#define QSURFACECAPTUREGRABBER_P_H

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

#include <QtMultimedia/qvideoframe.h>
#include <QtMultimedia/private/qplatformsurfacecapture_p.h>
#include <QtMultimedia/qtmultimediaglobal.h>

#include <QtCore/qmutex.h>

#include <chrono>
#include <memory>
#include <optional>

QT_BEGIN_NAMESPACE

class QThread;

static constexpr qreal DefaultScreenCaptureFrameRate = 60.f;
static constexpr qreal MinScreenCaptureFrameRate = 1.f;

class Q_MULTIMEDIA_EXPORT QSurfaceCaptureGrabber : public QObject
{
    Q_OBJECT
public:
    enum ThreadPolicy {
        UseCurrentThread,
        CreateGrabbingThread,
    };

    ~QSurfaceCaptureGrabber() override;

    void start();
    void stop();

    template<typename Object, typename Method>
    void addFrameCallback(Object *object, Method method)
    {
        connect(this, &QSurfaceCaptureGrabber::frameGrabbed, object, method,
                Qt::DirectConnection);
    }

    void setFrameRate(std::optional<qreal>);
    qreal frameRate() const;

    // Owning thread before start(), grabbing thread after.
    qreal activeFrameRate() const;

Q_SIGNALS:
    void frameGrabbed(const QVideoFrame&);
    void errorUpdated(QPlatformSurfaceCapture::Error error, const QString &description);

protected:
    QSurfaceCaptureGrabber(ThreadPolicy threadPolicy = CreateGrabbingThread);

    void updateError(QPlatformSurfaceCapture::Error error, const QString &description = {});

    virtual QVideoFrame grabFrame() = 0;

    void updateTimerInterval();
    void setTimerInterval(std::chrono::nanoseconds interval, quint64 generation);

    virtual void initializeGrabbingContext();
    virtual void finalizeGrabbingContext();

    bool isGrabbingContextInitialized() const;

    void injectContextToGrabbingThread(QObject *context);

private:
    class GrabbingProfiler;
    struct GrabbingContext;
    class GrabbingThread;

    mutable QMutex m_mutex;
    std::unique_ptr<GrabbingContext> m_context;
    std::optional<QPlatformSurfaceCapture::Error> m_prevError;
    std::unique_ptr<QThread> m_thread;
    std::chrono::nanoseconds m_timerInterval{ 0 };
    quint64 m_contextGeneration{ 0 };

    qreal m_rate{ DefaultScreenCaptureFrameRate };
    // Locked in start()/updateTimerInterval(); lock-free on the grabbing thread.
    qreal m_activeRate{ DefaultScreenCaptureFrameRate };
};

QT_END_NAMESPACE

#endif // QSURFACECAPTUREGRABBER_P_H
