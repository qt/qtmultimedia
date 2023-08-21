// Copyright (C) 2016 Research In Motion
// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#undef QT_NO_CONTEXTLESS_CONNECT // Remove after porting connect() calls

#include "qqnxmediarecorder_p.h"

#include "qqnxplatformcamera_p.h"
#include "qqnxaudioinput_p.h"
#include "qqnxcamera_p.h"
#include "qqnxmediacapturesession_p.h"

#include <private/qplatformcamera_p.h>

#include <QDebug>
#include <QUrl>

QT_BEGIN_NAMESPACE

QQnxMediaRecorder::QQnxMediaRecorder(QMediaRecorder *parent)
    : QPlatformMediaRecorder(parent)
{
}

bool QQnxMediaRecorder::isLocationWritable(const QUrl &/*location*/) const
{
    return true;
}

void QQnxMediaRecorder::setCaptureSession(QQnxMediaCaptureSession *session)
{
    m_session = session;
}

void QQnxMediaRecorder::record(QMediaEncoderSettings &settings)
{
    if (!m_session)
        return;

    m_audioRecorder.disconnect();

    if (hasCamera()) {
        startVideoRecording(settings);
    } else {
        QObject::connect(&m_audioRecorder, &QQnxAudioRecorder::durationChanged,
                [this](qint64 d) { durationChanged(d); });

        QObject::connect(&m_audioRecorder, &QQnxAudioRecorder::stateChanged,
                [this](QMediaRecorder::RecorderState s) { stateChanged(s); });

        QObject::connect(&m_audioRecorder, &QQnxAudioRecorder::actualLocationChanged,
                [this](const QUrl &l) { actualLocationChanged(l); });

        startAudioRecording(settings);
    }
}

void QQnxMediaRecorder::stop()
{
    if (hasCamera()) {
        stopVideoRecording();
    } else {
        m_audioRecorder.stop();
    }
}

void QQnxMediaRecorder::startAudioRecording(QMediaEncoderSettings &settings)
{
    if (!m_session)
        return;

    QQnxAudioInput *audioInput = m_session->audioInput();

    if (!audioInput)
        return;

    m_audioRecorder.setInputDeviceId(audioInput->device.id());
    m_audioRecorder.setMediaEncoderSettings(settings);
    m_audioRecorder.setOutputUrl(outputLocation());
    m_audioRecorder.record();
}

void QQnxMediaRecorder::startVideoRecording(QMediaEncoderSettings &settings)
{
    if (!hasCamera())
        return;

    auto *camera = static_cast<QQnxPlatformCamera*>(m_session->camera());

    camera->setMediaEncoderSettings(settings);
    camera->setOutputUrl(outputLocation());

    if (camera->startVideoRecording())
        stateChanged(QMediaRecorder::RecordingState);
}

void QQnxMediaRecorder::stopVideoRecording()
{
    if (!hasCamera())
        return;

    auto *camera = static_cast<QQnxPlatformCamera*>(m_session->camera());

    camera->stop();

    stateChanged(QMediaRecorder::StoppedState);
}

bool QQnxMediaRecorder::hasCamera() const
{
    return m_session && m_session->camera();
}

QT_END_NAMESPACE
