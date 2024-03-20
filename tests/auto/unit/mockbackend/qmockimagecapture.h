// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QMOCKCAMERAIMAGECAPTURE_H
#define QMOCKCAMERAIMAGECAPTURE_H

#include <QDateTime>
#include <QTimer>
#include <QtMultimedia/qmediametadata.h>

#include "private/qplatformimagecapture_p.h"
#include "private/qplatformcamera_p.h"

QT_BEGIN_NAMESPACE

class QMockMediaCaptureSession;

class QMockImageCapture : public QPlatformImageCapture
{
    Q_OBJECT
public:
    QMockImageCapture(QImageCapture *parent);

    ~QMockImageCapture()
    {
    }

    bool isReadyForCapture() const override;

    int capture(const QString &fileName) override;
    int captureToBuffer() override { return -1; }

    QImageEncoderSettings imageSettings() const override { return m_settings; }
    void setImageSettings(const QImageEncoderSettings &settings) override { m_settings = settings; }

private Q_SLOTS:
    void captured();

private:
    QString m_fileName;
    int m_captureRequest = 0;
    bool m_ready = true;
    QImageEncoderSettings m_settings;
};

QT_END_NAMESPACE

#endif // QMOCKCAMERAIMAGECAPTURE_H
