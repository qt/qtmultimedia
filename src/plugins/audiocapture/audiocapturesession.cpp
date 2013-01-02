/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore/qdebug.h>
#include <QtCore/qurl.h>
#include <QtCore/qdir.h>
#include <qaudiodeviceinfo.h>

#include "qmediarecorder.h"

#include "audiocapturesession.h"
#include "audiocaptureprobecontrol.h"

void FileProbeProxy::startProbes(const QAudioFormat &format)
{
    m_format = format;
}

void FileProbeProxy::stopProbes()
{
    m_format = QAudioFormat();
}

void FileProbeProxy::addProbe(AudioCaptureProbeControl *probe)
{
    QMutexLocker locker(&m_probeMutex);

    if (m_probes.contains(probe))
        return;

    m_probes.append(probe);
}

void FileProbeProxy::removeProbe(AudioCaptureProbeControl *probe)
{
    QMutexLocker locker(&m_probeMutex);
    m_probes.removeOne(probe);
}

qint64 FileProbeProxy::writeData(const char *data, qint64 len)
{
    if (m_format.isValid()) {
        QMutexLocker locker(&m_probeMutex);

        foreach (AudioCaptureProbeControl* probe, m_probes)
            probe->bufferProbed(data, len, m_format);
    }

    return QFile::writeData(data, len);
}

AudioCaptureSession::AudioCaptureSession(QObject *parent):
    QObject(parent)
{
    m_deviceInfo = new QAudioDeviceInfo(QAudioDeviceInfo::defaultInputDevice());
    m_audioInput = 0;
    m_position = 0;
    m_state = QMediaRecorder::StoppedState;

    m_format.setSampleRate(8000);
    m_format.setChannelCount(1);
    m_format.setSampleSize(8);
    m_format.setSampleType(QAudioFormat::UnSignedInt);
    m_format.setCodec("audio/pcm");
    wavFile = true;
}

AudioCaptureSession::~AudioCaptureSession()
{
    stop();

    if(m_audioInput)
        delete m_audioInput;
}

QAudioDeviceInfo* AudioCaptureSession::deviceInfo() const
{
    return m_deviceInfo;
}

QAudioFormat AudioCaptureSession::format() const
{
    return m_format;
}

bool AudioCaptureSession::isFormatSupported(const QAudioFormat &format) const
{
    if(m_deviceInfo) {
        if(format.codec().contains(QLatin1String("audio/x-wav"))) {
            QAudioFormat fmt = format;
            fmt.setCodec("audio/pcm");
            return m_deviceInfo->isFormatSupported(fmt);
        } else
            return m_deviceInfo->isFormatSupported(format);
    }
    return false;
}

bool AudioCaptureSession::setFormat(const QAudioFormat &format)
{
    if(m_deviceInfo) {

        QAudioFormat fmt = format;

        if(m_deviceInfo->isFormatSupported(fmt)) {
            m_format = fmt;
            if(m_audioInput) delete m_audioInput;
            m_audioInput = 0;
            QList<QAudioDeviceInfo> devices = QAudioDeviceInfo::availableDevices(QAudio::AudioInput);
            for(int i=0;i<devices.size();i++) {
                if(qstrcmp(m_deviceInfo->deviceName().toLocal8Bit().constData(),
                            devices.at(i).deviceName().toLocal8Bit().constData()) == 0) {
                    m_audioInput = new QAudioInput(devices.at(i),m_format);
                    connect(m_audioInput,SIGNAL(stateChanged(QAudio::State)),this,SLOT(stateChanged(QAudio::State)));
                    connect(m_audioInput,SIGNAL(notify()),this,SLOT(notify()));
                    break;
                }
            }
        } else {
            m_format = m_deviceInfo->preferredFormat();
            qWarning()<<"failed to setFormat using preferred...";
        }
    }
    return false;
}

QStringList AudioCaptureSession::supportedContainers() const
{
    QStringList list;
    if(m_deviceInfo) {
        if (m_deviceInfo->supportedCodecs().size() > 0) {
            list << "audio/x-wav";
            list << "audio/pcm";
        }
    }
    return list;
}

QString AudioCaptureSession::containerDescription(const QString &formatMimeType) const
{
    if(m_deviceInfo) {
        if (formatMimeType.contains(QLatin1String("audio/pcm")))
            return tr("RAW file format");
        if (formatMimeType.contains(QLatin1String("audio/x-wav")))
            return tr("WAV file format");
    }
    return QString();
}

void AudioCaptureSession::setContainerFormat(const QString &formatMimeType)
{
    if (!formatMimeType.contains(QLatin1String("audio/x-wav")) &&
        !formatMimeType.contains(QLatin1String("audio/pcm")) &&
        !formatMimeType.isEmpty())
        return;

    if(m_deviceInfo) {
        if (!m_deviceInfo->supportedCodecs().contains(QLatin1String("audio/pcm")))
            return;

        if (formatMimeType.isEmpty() || formatMimeType.contains(QLatin1String("audio/x-wav"))) {
            wavFile = true;
            m_format.setCodec("audio/pcm");
        } else {
            wavFile = false;
            m_format.setCodec(formatMimeType);
        }
    }
}

QString AudioCaptureSession::containerFormat() const
{
    if(wavFile)
        return QString("audio/x-wav");

    return QString("audio/pcm");
}

QUrl AudioCaptureSession::outputLocation() const
{
    return m_actualSink;
}

bool AudioCaptureSession::setOutputLocation(const QUrl& sink)
{
    m_sink = m_actualSink = sink;
    return true;
}

qint64 AudioCaptureSession::position() const
{
    return m_position;
}

int AudioCaptureSession::state() const
{
    return int(m_state);
}

QDir AudioCaptureSession::defaultDir() const
{
    QStringList dirCandidates;

#if defined(Q_WS_MAEMO_6)
    dirCandidates << QLatin1String("/home/user/MyDocs");
#endif

    dirCandidates << QDir::home().filePath("Documents");
    dirCandidates << QDir::home().filePath("My Documents");
    dirCandidates << QDir::homePath();
    dirCandidates << QDir::currentPath();
    dirCandidates << QDir::tempPath();

    foreach (const QString &path, dirCandidates) {
        QDir dir(path);
        if (dir.exists() && QFileInfo(path).isWritable())
            return dir;
    }

    return QDir();
}

QString AudioCaptureSession::generateFileName(const QDir &dir, const QString &ext) const
{

    int lastClip = 0;
    foreach(QString fileName, dir.entryList(QStringList() << QString("clip_*.%1").arg(ext))) {
        int imgNumber = fileName.mid(5, fileName.size()-6-ext.length()).toInt();
        lastClip = qMax(lastClip, imgNumber);
    }

    QString name = QString("clip_%1.%2").arg(lastClip+1,
                                     4, //fieldWidth
                                     10,
                                     QLatin1Char('0')).arg(ext);

    return dir.absoluteFilePath(name);
}

void AudioCaptureSession::record()
{
    if(!m_audioInput) {
        setFormat(m_format);
    }

    m_actualSink = m_sink;

    if (m_actualSink.isEmpty()) {
        QString ext = wavFile ? QLatin1String("wav") : QLatin1String("raw");
        m_actualSink = generateFileName(defaultDir(), ext);
    }

    if(m_actualSink.toLocalFile().length() > 0)
        file.setFileName(m_actualSink.toLocalFile());
    else
        file.setFileName(m_actualSink.toString());

    if(m_audioInput) {
        if(m_state == QMediaRecorder::StoppedState) {
            if(file.open(QIODevice::WriteOnly)) {
                memset(&header,0,sizeof(CombinedHeader));
                memcpy(header.riff.descriptor.id,"RIFF",4);
                header.riff.descriptor.size = 0xFFFFFFFF; // This should be updated on stop(), filesize-8
                memcpy(header.riff.type,"WAVE",4);
                memcpy(header.wave.descriptor.id,"fmt ",4);
                header.wave.descriptor.size = 16;
                header.wave.audioFormat = 1; // for PCM data
                header.wave.numChannels = m_format.channelCount();
                header.wave.sampleRate = m_format.sampleRate();
                header.wave.byteRate = m_format.sampleRate()*m_format.channelCount()*m_format.sampleSize()/8;
                header.wave.blockAlign = m_format.channelCount()*m_format.sampleSize()/8;
                header.wave.bitsPerSample = m_format.sampleSize();
                memcpy(header.data.descriptor.id,"data",4);
                header.data.descriptor.size = 0xFFFFFFFF; // This should be updated on stop(),samples*channels*sampleSize/8
                if (wavFile)
                    file.write((char*)&header,sizeof(CombinedHeader));

                file.startProbes(m_format);
                m_audioInput->start(qobject_cast<QIODevice*>(&file));
            } else {
                emit error(1,QString("can't open source, failed"));
                m_state = QMediaRecorder::StoppedState;
                emit stateChanged(m_state);
            }
        }
    }

    m_state = QMediaRecorder::RecordingState;
}

void AudioCaptureSession::pause()
{
    if(m_audioInput)
        m_audioInput->stop();

    m_state = QMediaRecorder::PausedState;
}

void AudioCaptureSession::stop()
{
    if(m_audioInput) {
        m_audioInput->stop();
        file.stopProbes();
        file.close();
        if (wavFile) {
            qint32 fileSize = file.size()-8;
            file.open(QIODevice::ReadWrite | QIODevice::Unbuffered);
            file.read((char*)&header,sizeof(CombinedHeader));
            header.riff.descriptor.size = fileSize; // filesize-8
            header.data.descriptor.size = fileSize-44; // samples*channels*sampleSize/8
            file.seek(0);
            file.write((char*)&header,sizeof(CombinedHeader));
            file.close();
        }
        m_position = 0;
    }
    m_state = QMediaRecorder::StoppedState;
}

void AudioCaptureSession::addProbe(AudioCaptureProbeControl *probe)
{
    file.addProbe(probe);
}

void AudioCaptureSession::removeProbe(AudioCaptureProbeControl *probe)
{
    file.removeProbe(probe);
}

void AudioCaptureSession::stateChanged(QAudio::State state)
{
    switch(state) {
        case QAudio::ActiveState:
            emit stateChanged(QMediaRecorder::RecordingState);
            break;
        default:
            if(!((m_state == QMediaRecorder::PausedState)||(m_state == QMediaRecorder::StoppedState)))
                m_state = QMediaRecorder::StoppedState;

            emit stateChanged(m_state);
            break;
    }
}

void AudioCaptureSession::notify()
{
    m_position += m_audioInput->notifyInterval();
    emit positionChanged(m_position);
}

void AudioCaptureSession::setCaptureDevice(const QString &deviceName)
{
    m_captureDevice = deviceName;
    if(m_deviceInfo)
        delete m_deviceInfo;

    m_deviceInfo = 0;

    QList<QAudioDeviceInfo> devices = QAudioDeviceInfo::availableDevices(QAudio::AudioInput);
    for(int i = 0; i < devices.size(); i++) {
        if(qstrcmp(m_captureDevice.toLocal8Bit().constData(),
                    devices.at(i).deviceName().toLocal8Bit().constData())==0){
            m_deviceInfo = new QAudioDeviceInfo(devices.at(i));
            return;
        }
    }
    m_deviceInfo = new QAudioDeviceInfo(QAudioDeviceInfo::defaultInputDevice());
}



