/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

#ifndef QWindowsImageCapture_H
#define QWindowsImageCapture_H

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

class QWindowsMediaDeviceSession;
class QWindowsMediaCaptureService;

class QWindowsImageCapture : public QPlatformImageCapture
{
    Q_OBJECT
public:
    explicit QWindowsImageCapture(QImageCapture *parent);
    virtual ~QWindowsImageCapture();

    bool isReadyForCapture() const override;

    int capture(const QString &fileName) override;
    int captureToBuffer() override;

    QImageEncoderSettings imageSettings() const override;
    void setImageSettings(const QImageEncoderSettings &settings) override;

    void setCaptureSession(QPlatformMediaCaptureSession *session);

private Q_SLOTS:
    void handleVideoFrameChanged(const QVideoFrame &frame);

private:
    int doCapture(const QString &fileName);
    void saveImage(int captureId, const QString &fileName,
                   const QImage &image, const QMediaMetaData &metaData,
                   const QImageEncoderSettings &settings);
    QString writerFormat(QImageCapture::FileFormat reqFormat);
    int writerQuality(const QString &writerFormat,
                      QImageCapture::Quality quality);

    QWindowsMediaCaptureService  *m_captureService = nullptr;
    QWindowsMediaDeviceSession   *m_mediaDeviceSession = nullptr;
    QImageEncoderSettings         m_settings;
    int m_captureId = 0;
    bool m_capturing = false;
    QString m_fileName;
};

QT_END_NAMESPACE

#endif  // QWindowsImageCapture_H
