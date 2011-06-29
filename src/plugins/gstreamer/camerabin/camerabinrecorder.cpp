/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Mobility Components.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "camerabinrecorder.h"
#include "camerabinaudioencoder.h"
#include "camerabinvideoencoder.h"
#include "camerabincontainer.h"
#include <QtCore/QDebug>

CameraBinRecorder::CameraBinRecorder(CameraBinSession *session)
    :QMediaRecorderControl(session),
     m_session(session),
     m_state(QMediaRecorder::StoppedState)
{
    connect(m_session, SIGNAL(stateChanged(QCamera::State)), SLOT(updateState()));
    connect(m_session, SIGNAL(durationChanged(qint64)), SIGNAL(durationChanged(qint64)));
    connect(m_session, SIGNAL(mutedChanged(bool)), this, SIGNAL(mutedChanged(bool)));
}

CameraBinRecorder::~CameraBinRecorder()
{
}

QUrl CameraBinRecorder::outputLocation() const
{
    return m_session->outputLocation();
}

bool CameraBinRecorder::setOutputLocation(const QUrl &sink)
{
    m_session->setOutputLocation(sink);
    return true;
}

QMediaRecorder::State CameraBinRecorder::state() const
{
    return m_state;
}

void CameraBinRecorder::updateState()
{
    if (m_session->state() != QCamera::ActiveState &&
            m_state != QMediaRecorder::StoppedState) {
        m_session->stopVideoRecording();
        emit stateChanged(m_state = QMediaRecorder::StoppedState);
    }
}

qint64 CameraBinRecorder::duration() const
{
    return m_session->duration();
}

void CameraBinRecorder::record()
{
    if (m_session->state() == QCamera::ActiveState) {
        if (m_state == QMediaRecorder::PausedState)
            m_session->resumeVideoRecording();
        else
            m_session->recordVideo();
        emit stateChanged(m_state = QMediaRecorder::RecordingState);
    } else
        emit error(QMediaRecorder::ResourceError, tr("Service has not been started"));
}

void CameraBinRecorder::pause()
{
    if (m_session->state() == QCamera::ActiveState) {
        m_session->pauseVideoRecording();
        emit stateChanged(m_state = QMediaRecorder::PausedState);
    } else
        emit error(QMediaRecorder::ResourceError, tr("Service has not been started"));
}

void CameraBinRecorder::stop()
{
    if (m_session->state() == QCamera::ActiveState) {
        m_session->stopVideoRecording();
        emit stateChanged(m_state = QMediaRecorder::StoppedState);
    }
}

bool CameraBinRecorder::findCodecs()
{
    //Check the codecs are compatible with container,
    //and choose the compatible codecs/container if omitted
    CameraBinAudioEncoder *audioEncodeControl = m_session->audioEncodeControl();
    CameraBinVideoEncoder *videoEncodeControl = m_session->videoEncodeControl();
    CameraBinContainer *mediaContainerControl = m_session->mediaContainerControl();

    audioEncodeControl->resetActualSettings();
    videoEncodeControl->resetActualSettings();
    mediaContainerControl->resetActualContainer();

    QStringList containerCandidates;
    if (mediaContainerControl->containerMimeType().isEmpty())
        containerCandidates = mediaContainerControl->supportedContainers();
    else
        containerCandidates << mediaContainerControl->containerMimeType();


    QStringList audioCandidates;
    QAudioEncoderSettings audioSettings = audioEncodeControl->audioSettings();
    if (audioSettings.codec().isEmpty())
        audioCandidates = audioEncodeControl->supportedAudioCodecs();
    else
        audioCandidates << audioSettings.codec();

    QStringList videoCandidates;
    QVideoEncoderSettings videoSettings = videoEncodeControl->videoSettings();
    if (videoSettings.codec().isEmpty())
        videoCandidates = videoEncodeControl->supportedVideoCodecs();
    else
        videoCandidates << videoSettings.codec();

    QString container;
    QString audioCodec;
    QString videoCodec;

    foreach (const QString &containerCandidate, containerCandidates) {
        QSet<QString> supportedTypes = mediaContainerControl->supportedStreamTypes(containerCandidate);

        audioCodec.clear();
        videoCodec.clear();

        bool found = false;
        foreach (const QString &audioCandidate, audioCandidates) {
            QSet<QString> audioTypes = audioEncodeControl->supportedStreamTypes(audioCandidate);
            if (!audioTypes.intersect(supportedTypes).isEmpty()) {
                found = true;
                audioCodec = audioCandidate;
                break;
            }
        }
        if (!found)
            continue;

        found = false;
        foreach (const QString &videoCandidate, videoCandidates) {
            QSet<QString> videoTypes = videoEncodeControl->supportedStreamTypes(videoCandidate);
            if (!videoTypes.intersect(supportedTypes).isEmpty()) {
                found = true;
                videoCodec = videoCandidate;
                break;
            }
        }
        if (!found)
            continue;


        container = containerCandidate;
        break;
    }

    if (container.isEmpty()) {
        qWarning() << "Camera error: Not compatible codecs and container format.";
        emit error(QMediaRecorder::FormatError, tr("Not compatible codecs and container format."));
        return false;
    } else {
        mediaContainerControl->setActualContainer(container);

        QAudioEncoderSettings audioSettings = audioEncodeControl->audioSettings();
        audioSettings.setCodec(audioCodec);
        audioEncodeControl->setActualAudioSettings(audioSettings);

        QVideoEncoderSettings videoSettings = videoEncodeControl->videoSettings();
        videoSettings.setCodec(videoCodec);
        videoEncodeControl->setActualVideoSettings(videoSettings);
    }

    return true;
}

void CameraBinRecorder::applySettings()
{
    findCodecs();
}

bool CameraBinRecorder::isMuted() const
{
    return m_session->isMuted();
}

void CameraBinRecorder::setMuted(bool muted)
{
    m_session->setMuted(muted);
}
