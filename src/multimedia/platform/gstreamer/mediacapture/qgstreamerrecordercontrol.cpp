/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "qgstreamerrecordercontrol_p.h"
#include "qgstreamercapturemetadatacontrol_p.h"
#include <QtCore/QDebug>
#include <QtGui/qdesktopservices.h>
#include <QStandardPaths>
#include "qaudiodeviceinfo.h"
#include <qmimetype.h>

QGstreamerRecorderControl::QGstreamerRecorderControl(QGstreamerCaptureSession *session)
    : QMediaRecorderControl(session),
      m_session(session),
      m_state(QMediaRecorder::StoppedState),
      m_status(QMediaRecorder::UnloadedStatus)
{
    connect(m_session, SIGNAL(stateChanged(QGstreamerCaptureSession::State)), SLOT(updateStatus()));
    connect(m_session, SIGNAL(error(int,QString)), SLOT(handleSessionError(int,QString)));
    connect(m_session, SIGNAL(durationChanged(qint64)), SIGNAL(durationChanged(qint64)));
    connect(m_session, SIGNAL(mutedChanged(bool)), SIGNAL(mutedChanged(bool)));
    connect(m_session, SIGNAL(volumeChanged(qreal)), SIGNAL(volumeChanged(qreal)));
    m_hasPreviewState = m_session->captureMode() != QGstreamerCaptureSession::Audio;

    m_metaData = new QGstreamerCaptureMetaDataControl(this);
    connect(m_metaData, SIGNAL(metaDataChanged(QMap<QByteArray,QVariant>)),
            m_session, SLOT(setMetaData(QMap<QByteArray,QVariant>)));
}

QGstreamerRecorderControl::~QGstreamerRecorderControl()
{
    delete m_metaData;
}

QUrl QGstreamerRecorderControl::outputLocation() const
{
    return m_session->outputLocation();
}

bool QGstreamerRecorderControl::setOutputLocation(const QUrl &sink)
{
    m_outputLocation = sink;
    m_session->setOutputLocation(sink);
    return true;
}


QMediaRecorder::State QGstreamerRecorderControl::state() const
{
    return m_state;
}

QMediaRecorder::Status QGstreamerRecorderControl::status() const
{
    static QMediaRecorder::Status statusTable[3][3] = {
        //Stopped recorder state:
        { QMediaRecorder::LoadedStatus, QMediaRecorder::FinalizingStatus, QMediaRecorder::FinalizingStatus },
        //Recording recorder state:
        { QMediaRecorder::StartingStatus, QMediaRecorder::RecordingStatus, QMediaRecorder::PausedStatus },
        //Paused recorder state:
        { QMediaRecorder::StartingStatus, QMediaRecorder::RecordingStatus, QMediaRecorder::PausedStatus }
    };

    QMediaRecorder::State sessionState = QMediaRecorder::StoppedState;

    switch ( m_session->state() ) {
        case QGstreamerCaptureSession::RecordingState:
            sessionState = QMediaRecorder::RecordingState;
            break;
        case QGstreamerCaptureSession::PausedState:
            sessionState = QMediaRecorder::PausedState;
            break;
        case QGstreamerCaptureSession::PreviewState:
        case QGstreamerCaptureSession::StoppedState:
            sessionState = QMediaRecorder::StoppedState;
            break;
    }

    return statusTable[m_state][sessionState];
}

void QGstreamerRecorderControl::updateStatus()
{
    QMediaRecorder::Status newStatus = status();
    if (m_status != newStatus) {
        m_status = newStatus;
        emit statusChanged(m_status);
        // If stop has been called and session state became stopped.
        if (m_status == QMediaRecorder::LoadedStatus)
            emit stateChanged(m_state);
    }
}

void QGstreamerRecorderControl::handleSessionError(int code, const QString &description)
{
    emit error(code, description);
    stop();
}

qint64 QGstreamerRecorderControl::duration() const
{
    return m_session->duration();
}

void QGstreamerRecorderControl::setState(QMediaRecorder::State state)
{
    switch (state) {
    case QMediaRecorder::StoppedState:
        stop();
        break;
    case QMediaRecorder::PausedState:
        pause();
        break;
    case QMediaRecorder::RecordingState:
        record();
        break;
    }
}

void QGstreamerRecorderControl::record()
{
    if (m_state == QMediaRecorder::RecordingState)
        return;

    m_state = QMediaRecorder::RecordingState;

    if (m_outputLocation.isEmpty()) {
        QString container = resolvedEncoderSettings().mimeType().preferredSuffix();
        if (container.isEmpty())
            container = QString::fromLatin1("raw");

        m_session->setOutputLocation(QUrl(generateFileName(defaultDir(), container)));
    }

    m_session->dumpGraph("before-record");
    if (!m_hasPreviewState || m_session->state() != QGstreamerCaptureSession::StoppedState) {
        m_session->setState(QGstreamerCaptureSession::RecordingState);
    } else
        emit error(QMediaRecorder::ResourceError, tr("Service has not been started"));

    m_session->dumpGraph("after-record");

    emit stateChanged(m_state);
    updateStatus();

    emit actualLocationChanged(m_session->outputLocation());
}

void QGstreamerRecorderControl::pause()
{
    if (m_state == QMediaRecorder::PausedState)
        return;

    m_state = QMediaRecorder::PausedState;

    m_session->dumpGraph("before-pause");
    if (!m_hasPreviewState || m_session->state() != QGstreamerCaptureSession::StoppedState) {
        m_session->setState(QGstreamerCaptureSession::PausedState);
    } else
        emit error(QMediaRecorder::ResourceError, tr("Service has not been started"));

    emit stateChanged(m_state);
    updateStatus();
}

void QGstreamerRecorderControl::stop()
{
    if (m_state == QMediaRecorder::StoppedState)
        return;

    m_state = QMediaRecorder::StoppedState;

    if (!m_hasPreviewState) {
        m_session->setState(QGstreamerCaptureSession::StoppedState);
    } else {
        if (m_session->state() != QGstreamerCaptureSession::StoppedState)
            m_session->setState(QGstreamerCaptureSession::PreviewState);
    }

    updateStatus();
}

void QGstreamerRecorderControl::applySettings()
{
}

QAudioDeviceInfo QGstreamerRecorderControl::audioInput() const
{
    return m_session->audioCaptureDevice();
}

bool QGstreamerRecorderControl::setAudioInput(const QAudioDeviceInfo &info)
{
    m_session->setAudioCaptureDevice(info);
    return true;
}

void QGstreamerRecorderControl::setEncoderSettings(const QMediaEncoderSettings &settings)
{
    m_settings = settings;
}

QMediaEncoderSettings QGstreamerRecorderControl::resolvedEncoderSettings() const
{
    QMediaEncoderSettings f = m_settings;
    f.resolveFormat();
    return f;
}

QMetaDataWriterControl *QGstreamerRecorderControl::metaDataControl()
{
    return m_metaData;
}

bool QGstreamerRecorderControl::isMuted() const
{
    return m_session->isMuted();
}

qreal QGstreamerRecorderControl::volume() const
{
    return m_session->volume();
}

void QGstreamerRecorderControl::setMuted(bool muted)
{
    m_session->setMuted(muted);
}

void QGstreamerRecorderControl::setVolume(qreal volume)
{
    m_session->setVolume(volume);
}

QDir QGstreamerRecorderControl::defaultDir() const
{
    QStringList dirCandidates;

    if (m_session->captureMode() & QGstreamerCaptureSession::Video)
        dirCandidates << QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
    else
        dirCandidates << QStandardPaths::writableLocation(QStandardPaths::MusicLocation);

    dirCandidates << QDir::home().filePath("Documents");
    dirCandidates << QDir::home().filePath("My Documents");
    dirCandidates << QDir::homePath();
    dirCandidates << QDir::currentPath();
    dirCandidates << QDir::tempPath();

    for (const QString &path : qAsConst(dirCandidates)) {
        QDir dir(path);
        if (dir.exists() && QFileInfo(path).isWritable())
            return dir;
    }

    return QDir();
}

QString QGstreamerRecorderControl::generateFileName(const QDir &dir, const QString &ext) const
{

    int lastClip = 0;
    const auto list = dir.entryList(QStringList() << QString("clip_*.%1").arg(ext));
    for (const QString &fileName : list) {
        int imgNumber = QStringView{fileName}.mid(5, fileName.size()-6-ext.length()).toInt();
        lastClip = qMax(lastClip, imgNumber);
    }

    QString name = QString("clip_%1.%2").arg(lastClip+1,
                                     4, //fieldWidth
                                     10,
                                     QLatin1Char('0')).arg(ext);

    return dir.absoluteFilePath(name);
}
