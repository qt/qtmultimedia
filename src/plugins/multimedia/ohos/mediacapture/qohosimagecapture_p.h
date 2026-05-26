// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSIMAGECAPTURE_P_H
#define QOHOSIMAGECAPTURE_P_H

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

#include <QtCore/qpointer.h>

#include <private/qplatformimagecapture_p.h>

QT_BEGIN_NAMESPACE

class QOhosCameraSession;
class QOhosMediaCaptureSession;

class QOhosImageCapture : public QPlatformImageCapture
{
    Q_OBJECT
public:
    explicit QOhosImageCapture(QImageCapture *parent);

    bool isReadyForCapture() const override;
    int capture(const QString &fileName) override;
    int captureToBuffer() override;

    QImageEncoderSettings imageSettings() const override;
    void setImageSettings(const QImageEncoderSettings &settings) override;

    void setCaptureSession(QPlatformMediaCaptureSession *session);

private:
    void connectToSession();
    void disconnectFromSession();
    void notifyReadyForCaptureChanged(bool ready);
    void onSessionImageCaptured(int id, const QImage &preview);

    QPointer<QOhosMediaCaptureSession> m_service;
    QPointer<QOhosCameraSession> m_session;
    QImageEncoderSettings m_pendingSettings;
    bool m_lastReady{ false };
};

QT_END_NAMESPACE

#endif // QOHOSIMAGECAPTURE_P_H
