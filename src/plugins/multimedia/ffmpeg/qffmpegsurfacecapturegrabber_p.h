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

static constexpr qreal DefaultScreenCaptureFrameRate = 60.;

// Mac screens often support 120 frames per sec; it looks, this is not
// needed for the capturing now since it just affects CPI without valuable
// advantages. In the future, the frame rate should be customized by
// user's API.
static constexpr qreal MaxScreenCaptureFrameRate = 60.;
static constexpr qreal MinScreenCaptureFrameRate = 1.;

class QFFmpegSurfaceCaptureGrabber : public QObject
{
    Q_OBJECT
public:
    enum ThreadPolicy {
        UseCurrentThread,
        CreateGrabbingThread,
    };

    QFFmpegSurfaceCaptureGrabber(ThreadPolicy threadPolicy = CreateGrabbingThread);

    ~QFFmpegSurfaceCaptureGrabber() override;

    void start();
    void stop();

    template<typename Object, typename Method>
    void addFrameCallback(Object &object, Method method)
    {
        connect(this, &QFFmpegSurfaceCaptureGrabber::frameGrabbed,
                &object, method, Qt::DirectConnection);
    }

signals:
    void frameGrabbed(const QVideoFrame&);
    void errorUpdated(QPlatformSurfaceCapture::Error error, const QString &description);

protected:
    void updateError(QPlatformSurfaceCapture::Error error, const QString &description = {});

    virtual QVideoFrame grabFrame() = 0;

    void setFrameRate(qreal rate);

    qreal frameRate() const;

    void updateTimerInterval();

    virtual void initializeGrabbingContext();
    virtual void finalizeGrabbingContext();

    bool isGrabbingContextInitialized() const;

private:
    struct GrabbingContext;
    class GrabbingThread;

    std::unique_ptr<GrabbingContext> m_context;
    qreal m_rate = 0;
    std::optional<QPlatformSurfaceCapture::Error> m_prevError;
    std::unique_ptr<QThread> m_thread;
};

QT_END_NAMESPACE

#endif // QFFMPEGSURFACECAPTUREGRABBER_P_H
