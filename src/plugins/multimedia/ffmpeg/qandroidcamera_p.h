// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QANDROIDCAMERA_H
#define QANDROIDCAMERA_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qplatformcamera_p.h>
#include <QObject>
#include <QJniObject>

QT_BEGIN_NAMESPACE

class QAndroidCamera : public QPlatformCamera
{
    Q_OBJECT
public:
    enum State { Closed, WaitingOpen, WaitingStart, Started };
    explicit QAndroidCamera(QCamera *camera);
    ~QAndroidCamera() override;

    bool isActive() const override { return m_state == State::Started; }
    void setActive(bool active) override;
    void setCamera(const QCameraDevice &camera) override;
    bool setCameraFormat(const QCameraFormat &format) override;

    static bool registerNativeMethods();

public slots:
    void onCameraOpened();
    void onCameraDisconnect();
    void onCameraError(int error);
    void onFrameAvailable(QJniObject frame);
    void onCaptureSessionConfigured();
    void onCaptureSessionConfigureFailed();
    void onCaptureSessionStarted(long timestamp, long frameNumber);
    void onCaptureSessionCompleted(long frameNumber);
    void onCaptureSessionFailed(int reason, long frameNumber);

private:
    void setState(State newState);
    int orientation();

    State m_state = State::Closed;
    QCameraDevice m_cameraDevice;
    long lastTimestamp = 0;
    QJniObject m_jniCamera;
};

QT_END_NAMESPACE

#endif // QANDROIDCAMERA_H
