// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QANDROIDCAMERAIMAGECAPTURECONTROL_H
#define QANDROIDCAMERAIMAGECAPTURECONTROL_H

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

#include <private/qplatformimagecapture_p.h>

QT_BEGIN_NAMESPACE

class QAndroidCameraSession;
class QAndroidMediaCaptureSession;

class QAndroidImageCapture : public QPlatformImageCapture
{
    Q_OBJECT
public:
    explicit QAndroidImageCapture(QImageCapture *parent = nullptr);

    bool isReadyForCapture() const override;

    int capture(const QString &fileName) override;
    int captureToBuffer() override;

    QImageEncoderSettings imageSettings() const override;
    void setImageSettings(const QImageEncoderSettings &settings) override;

    void setCaptureSession(QPlatformMediaCaptureSession *session);

private:
    QAndroidCameraSession *m_session;
    QAndroidMediaCaptureSession *m_service;
};

QT_END_NAMESPACE

#endif // QANDROIDCAMERAIMAGECAPTURECONTROL_H
