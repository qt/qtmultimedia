// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWASMIMAGECAPTURE_H
#define QWASMIMAGECAPTURE_H

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

#include <QObject>
#include <private/qplatformimagecapture_p.h>
#include <QtCore/qloggingcategory.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(qWasmImageCapture)

class QWasmMediaCaptureSession;

class QWasmImageCapture : public QPlatformImageCapture
{
    Q_OBJECT
public:
    explicit QWasmImageCapture(QImageCapture *parent = nullptr);
    ~QWasmImageCapture();

    bool isReadyForCapture() const override;

    int capture(const QString &fileName) override;
    int captureToBuffer() override;

    QImageEncoderSettings imageSettings() const override;
    void setImageSettings(const QImageEncoderSettings &settings) override;

    void setReadyForCapture(bool isReady);

    void setCaptureSession(QPlatformMediaCaptureSession *session);

private:
    QImage takePicture();

    // weak
    QWasmMediaCaptureSession *m_captureSession = nullptr;
    QImageEncoderSettings m_settings;
    bool m_isReadyForCapture = false;
    int m_lastId = 0;
};

QT_END_NAMESPACE
#endif // QWASMIMAGECAPTURE_H
