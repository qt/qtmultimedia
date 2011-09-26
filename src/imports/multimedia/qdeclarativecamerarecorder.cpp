/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qdeclarativecamera_p.h"
#include "qdeclarativecamerarecorder_p.h"

#include <QtCore/qurl.h>

QT_BEGIN_NAMESPACE

QDeclarativeCameraRecorder::QDeclarativeCameraRecorder(QCamera *camera, QObject *parent) :
    QObject(parent)
{
    m_recorder = new QMediaRecorder(camera, this);
    connect(m_recorder, SIGNAL(stateChanged(QMediaRecorder::State)),
            SLOT(updateRecorderState(QMediaRecorder::State)));
    connect(m_recorder, SIGNAL(error(QMediaRecorder::Error)),
            SLOT(updateRecorderError(QMediaRecorder::Error)));
    connect(m_recorder, SIGNAL(mutedChanged(bool)), SIGNAL(mutedChanged(bool)));
    connect(m_recorder, SIGNAL(durationChanged(qint64)), SIGNAL(durationChanged(qint64)));
}

QDeclarativeCameraRecorder::~QDeclarativeCameraRecorder()
{
}

QSize QDeclarativeCameraRecorder::captureResolution()
{
    return m_videoSettings.resolution();
}

QString QDeclarativeCameraRecorder::audioCodec() const
{
    return m_audioSettings.codec();
}

QString QDeclarativeCameraRecorder::videoCodec() const
{
    return m_videoSettings.codec();
}

QString QDeclarativeCameraRecorder::mediaContainer() const
{
    return m_mediaContainer;
}

void QDeclarativeCameraRecorder::setCaptureResolution(const QSize &resolution)
{
    if (resolution != captureResolution()) {
        m_videoSettings.setResolution(resolution);
        applySettings();
        emit captureResolutionChanged(resolution);
    }
}

void QDeclarativeCameraRecorder::setAudioCodec(const QString &codec)
{
    if (codec != audioCodec()) {
        m_audioSettings.setCodec(codec);
        applySettings();
        emit audioCodecChanged(codec);
    }
}

void QDeclarativeCameraRecorder::setVideoCodec(const QString &codec)
{
    if (codec != videoCodec()) {
        m_videoSettings.setCodec(codec);
        applySettings();
        emit videoCodecChanged(codec);
    }
}

void QDeclarativeCameraRecorder::setMediaContainer(const QString &container)
{
    if (container != m_mediaContainer) {
        m_mediaContainer = container;
        applySettings();
        emit mediaContainerChanged(container);
    }
}

void QDeclarativeCameraRecorder::applySettings()
{
    m_recorder->setEncodingSettings(m_audioSettings,
                                    m_videoSettings,
                                    m_mediaContainer);
}

QMediaRecorder::Error QDeclarativeCameraRecorder::error() const
{
    return m_recorder->error();
}

QString QDeclarativeCameraRecorder::errorString() const
{
    return m_recorder->errorString();
}

QDeclarativeCameraRecorder::RecorderState QDeclarativeCameraRecorder::recorderState() const
{
    //paused state is not supported for camera
    QMediaRecorder::State state = m_recorder->state();

    if (state == QMediaRecorder::PausedState)
        state = QMediaRecorder::StoppedState;

    return RecorderState(state);
}

void QDeclarativeCameraRecorder::record()
{
    setRecorderState(RecordingState);
}

void QDeclarativeCameraRecorder::stop()
{
    setRecorderState(StoppedState);
}

void QDeclarativeCameraRecorder::setRecorderState(QDeclarativeCameraRecorder::RecorderState state)
{
    if (!m_recorder)
        return;

    switch (state) {
    case QDeclarativeCameraRecorder::RecordingState:
        m_recorder->record();
        break;
    case QDeclarativeCameraRecorder::StoppedState:
        m_recorder->stop();
        break;
    }
}

QString QDeclarativeCameraRecorder::outputLocation() const
{
    return m_recorder->outputLocation().toString();
}

void QDeclarativeCameraRecorder::setOutputLocation(const QUrl &location)
{
    m_recorder->setOutputLocation(location);
}

qint64 QDeclarativeCameraRecorder::duration() const
{
    return m_recorder->duration();
}

bool QDeclarativeCameraRecorder::isMuted() const
{
    return m_recorder->isMuted();
}

void QDeclarativeCameraRecorder::setMuted(bool muted)
{
    m_recorder->setMuted(muted);
}

void QDeclarativeCameraRecorder::setMetadata(const QString &key, const QVariant &value)
{
    m_recorder->setExtendedMetaData(key, value);
}

void QDeclarativeCameraRecorder::updateRecorderState(QMediaRecorder::State state)
{
    if (state == QMediaRecorder::PausedState)
        state = QMediaRecorder::StoppedState;

    if (state == QMediaRecorder::StoppedState) {
        QString location = outputLocation();
        if (!location.isEmpty())
            emit outputLocationChanged(location);
    }

    emit recorderStateChanged(RecorderState(state));
}

void QDeclarativeCameraRecorder::updateRecorderError(QMediaRecorder::Error errorCode)
{
    qWarning() << "QMediaRecorder error:" << errorString();
    emit error(errorCode);
}

QT_END_NAMESPACE

#include "moc_qdeclarativecamerarecorder_p.cpp"
