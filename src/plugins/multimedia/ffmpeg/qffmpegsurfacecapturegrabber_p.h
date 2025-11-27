// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QFFMPEGSURFACECAPTUREGRABBER_P_H
#define QFFMPEGSURFACECAPTUREGRABBER_P_H

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

#include "qvideoframe.h"
#include <QtMultimedia/private/qplatformsurfacecapture_p.h>

#include <memory>
#include <optional>

QT_BEGIN_NAMESPACE

class QThread;

static constexpr qreal DefaultScreenCaptureFrameRate = 60.f;
static constexpr qreal MinScreenCaptureFrameRate = 1.f;

class QFFmpegSurfaceCaptureGrabber : public QObject
{
    Q_OBJECT
public:
    enum ThreadPolicy {
        UseCurrentThread,
        CreateGrabbingThread,
    };

    ~QFFmpegSurfaceCaptureGrabber() override;

    void start();
    void stop();

    template<typename Object, typename Method>
    void addFrameCallback(Object *object, Method method)
    {
        connect(this, &QFFmpegSurfaceCaptureGrabber::frameGrabbed, object, method,
                Qt::DirectConnection);
    }

    void setFrameRate(std::optional<qreal>);
    qreal frameRate() const;

signals:
    void frameGrabbed(const QVideoFrame&);
    void errorUpdated(QPlatformSurfaceCapture::Error error, const QString &description);

protected:
    QFFmpegSurfaceCaptureGrabber(ThreadPolicy threadPolicy = CreateGrabbingThread);

    void updateError(QPlatformSurfaceCapture::Error error, const QString &description = {});

    virtual QVideoFrame grabFrame() = 0;

    void updateTimerInterval();

    virtual void initializeGrabbingContext();
    virtual void finalizeGrabbingContext();

    bool isGrabbingContextInitialized() const;

private:
    class GrabbingProfiler;
    struct GrabbingContext;
    class GrabbingThread;

    std::unique_ptr<GrabbingContext> m_context;
    std::optional<QPlatformSurfaceCapture::Error> m_prevError;
    std::unique_ptr<QThread> m_thread;

    qreal m_rate{ DefaultScreenCaptureFrameRate };
};

QT_END_NAMESPACE

#endif // QFFMPEGSURFACECAPTUREGRABBER_P_H
