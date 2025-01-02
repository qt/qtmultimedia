// Copyright (C) 2016 Research In Motion
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QQnxImageCapture_H
#define QQnxImageCapture_H

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

#include <QtCore/qfuture.h>

QT_BEGIN_NAMESPACE

class QQnxMediaCaptureSession;
class QQnxPlatformCamera;

class QThread;

class QQnxImageCapture : public QPlatformImageCapture
{
    Q_OBJECT
public:
    explicit QQnxImageCapture(QImageCapture *parent);

    bool isReadyForCapture() const override;

    int capture(const QString &fileName) override;
    int captureToBuffer() override;

    QImageEncoderSettings imageSettings() const override;
    void setImageSettings(const QImageEncoderSettings &settings) override;

    void setCaptureSession(QQnxMediaCaptureSession *session);

private:
    QFuture<QImage> decodeFrame(int id, const QVideoFrame &frame);
    void saveFrame(int id, const QVideoFrame &frame, const QString &fileName);

    void onCameraChanged();
    void onCameraActiveChanged(bool active);
    void updateReadyForCapture();

    QQnxMediaCaptureSession *m_session = nullptr;
    QQnxPlatformCamera *m_camera = nullptr;

    int m_lastId = 0;
    QImageEncoderSettings m_settings;

    bool m_lastReadyForCapture = false;
};

QT_END_NAMESPACE

#endif
