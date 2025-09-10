// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QANDROIDMEDIAENCODER_H
#define QANDROIDMEDIAENCODER_H

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

#include <private/qplatformmediarecorder_p.h>
#include <private/qplatformmediacapture_p.h>

QT_BEGIN_NAMESPACE

class QAndroidCaptureSession;
class QAndroidMediaCaptureSession;

class QAndroidMediaEncoder : public QPlatformMediaRecorder
{
public:
    explicit QAndroidMediaEncoder(QMediaRecorder *parent);

    bool isLocationWritable(const QUrl &location) const override;
    QMediaRecorder::RecorderState state() const override;
    qint64 duration() const override;

    void setCaptureSession(QPlatformMediaCaptureSession *session);

    void setOutputLocation(const QUrl &location) override;
    void record(QMediaEncoderSettings &settings) override;
    void stop() override;

private:
    friend class QAndroidCaptureSession;

    QAndroidCaptureSession *m_session = nullptr;
    QAndroidMediaCaptureSession *m_service = nullptr;
};

QT_END_NAMESPACE

#endif // QANDROIDMEDIAENCODER_H
