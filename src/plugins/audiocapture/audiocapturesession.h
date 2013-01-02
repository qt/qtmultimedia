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

#ifndef AUDIOCAPTURESESSION_H
#define AUDIOCAPTURESESSION_H

#include <QFile>
#include <QUrl>
#include <QDir>
#include <QMutex>

#include "audioencodercontrol.h"
#include "audioinputselector.h"
#include "audiomediarecordercontrol.h"

#include <qaudioformat.h>
#include <qaudioinput.h>
#include <qaudiodeviceinfo.h>

QT_USE_NAMESPACE

class AudioCaptureProbeControl;

class FileProbeProxy: public QFile {
public:
    void startProbes(const QAudioFormat& format);
    void stopProbes();
    void addProbe(AudioCaptureProbeControl *probe);
    void removeProbe(AudioCaptureProbeControl *probe);

protected:
    virtual qint64 writeData(const char *data, qint64 len);

private:
    QAudioFormat m_format;
    QList<AudioCaptureProbeControl*> m_probes;
    QMutex m_probeMutex;
};


class AudioCaptureSession : public QObject
{
    Q_OBJECT

public:
    AudioCaptureSession(QObject *parent = 0);
    ~AudioCaptureSession();

    QAudioFormat format() const;
    QAudioDeviceInfo* deviceInfo() const;
    bool isFormatSupported(const QAudioFormat &format) const;
    bool setFormat(const QAudioFormat &format);
    QStringList supportedContainers() const;
    QString containerFormat() const;
    void setContainerFormat(const QString &formatMimeType);
    QString containerDescription(const QString &formatMimeType) const;

    QUrl outputLocation() const;
    bool setOutputLocation(const QUrl& sink);
    qint64 position() const;
    int state() const;
    void record();
    void pause();
    void stop();
    void addProbe(AudioCaptureProbeControl *probe);
    void removeProbe(AudioCaptureProbeControl *probe);

public slots:
    void setCaptureDevice(const QString &deviceName);

signals:
    void stateChanged(QMediaRecorder::State state);
    void positionChanged(qint64 position);
    void error(int error, const QString &errorString);

private slots:
    void stateChanged(QAudio::State state);
    void notify();

private:
    QDir defaultDir() const;
    QString generateFileName(const QDir &dir, const QString &ext) const;

    FileProbeProxy file;
    QString m_captureDevice;
    QUrl m_sink;
    QUrl m_actualSink;
    QMediaRecorder::State m_state;
    QAudioInput *m_audioInput;
    QAudioDeviceInfo *m_deviceInfo;
    QAudioFormat m_format;
    qint64 m_position;
    bool wavFile;

    // WAV header stuff

    struct chunk
    {
        char        id[4];
        quint32     size;
    };

    struct RIFFHeader
    {
        chunk       descriptor;
        char        type[4];
    };

    struct WAVEHeader
    {
        chunk       descriptor;
        quint16     audioFormat;        // PCM = 1
        quint16     numChannels;
        quint32     sampleRate;
        quint32     byteRate;
        quint16     blockAlign;
        quint16     bitsPerSample;
    };

    struct DATAHeader
    {
        chunk       descriptor;
//        quint8      data[];
    };

    struct CombinedHeader
    {
        RIFFHeader  riff;
        WAVEHeader  wave;
        DATAHeader  data;
    };

    CombinedHeader      header;
};

#endif
