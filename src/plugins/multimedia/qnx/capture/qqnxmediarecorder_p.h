// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QQNXMEDIARECORDER_H
#define QQNXMEDIARECORDER_H

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

#include "qqnxaudiorecorder_p.h"

#include <private/qplatformmediarecorder_p.h>

QT_BEGIN_NAMESPACE

class QQnxMediaCaptureSession;

class QQnxMediaRecorder : public QPlatformMediaRecorder
{
public:
    explicit QQnxMediaRecorder(QMediaRecorder *parent);

    bool isLocationWritable(const QUrl &location) const override;

    void record(QMediaEncoderSettings &settings) override;
    void stop() override;

    void setCaptureSession(QQnxMediaCaptureSession *session);

private:
    bool hasCamera() const;

    void startAudioRecording(QMediaEncoderSettings &settings);
    void startVideoRecording(QMediaEncoderSettings &settings);
    void stopVideoRecording();

    QQnxAudioRecorder m_audioRecorder;

    QQnxMediaCaptureSession *m_session = nullptr;
};

QT_END_NAMESPACE

#endif
