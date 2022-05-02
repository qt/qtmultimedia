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
