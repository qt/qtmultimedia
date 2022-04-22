/****************************************************************************
**
** Copyright (C) 2016 Research In Motion
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include "qqnxmediarecorder_p.h"

#include "qqnxaudioinput_p.h"
#include "qqnxmediacapturesession_p.h"

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

    if (hasVideo()) {
        //FIXME
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
    if (hasVideo()) {
        //FIXME
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

bool QQnxMediaRecorder::hasVideo() const
{
    return m_session->camera();
}

QT_END_NAMESPACE
