// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QFFMPEGSURFACECAPTURETHREAD_P_H
#define QFFMPEGSURFACECAPTURETHREAD_P_H

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

#include <qthread.h>

#include "qvideoframe.h"
#include "private/qplatformsurfacecapture_p.h"

#include <memory>
#include <optional>

QT_BEGIN_NAMESPACE

class QTimer;

static constexpr qreal DefaultScreenCaptureFrameRate = 60.;

// Mac screens often support 120 frames per sec; it looks, this is not
// needed for the capturing now since it just affects CPI without valuable
// advantages. In the future, the frame rate should be customized by
// user's API.
static constexpr qreal MaxScreenCaptureFrameRate = 60.;
static constexpr qreal MinScreenCaptureFrameRate = 1.;

class QFFmpegSurfaceCaptureThread : public QThread
{
    Q_OBJECT
public:
    QFFmpegSurfaceCaptureThread();

    ~QFFmpegSurfaceCaptureThread() override;

    void stop();

    template<typename Object, typename Method>
    void addFrameCallback(Object &object, Method method)
    {
        connect(this, &QFFmpegSurfaceCaptureThread::frameGrabbed,
                &object, method, Qt::DirectConnection);
    }

signals:
    void frameGrabbed(const QVideoFrame&);
    void errorUpdated(QPlatformSurfaceCapture::Error error, const QString &description);

protected:
    void run() override;

    void updateError(QPlatformSurfaceCapture::Error error, const QString &description = {});

    virtual QVideoFrame grabFrame() = 0;

protected:
    void setFrameRate(qreal rate);

    qreal frameRate() const;

    void updateTimerInterval();

private:
    qreal m_rate = 0;
    std::unique_ptr<QTimer> m_timer;
    std::optional<QPlatformSurfaceCapture::Error> m_prevError;
};

QT_END_NAMESPACE

#endif // QFFMPEGSURFACECAPTURETHREAD_P_H
