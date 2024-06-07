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
#ifndef IOSAUDIOOUTPUT_H
#define IOSAUDIOOUTPUT_H

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

#include <qaudiosystem_p.h>

#if defined(Q_OS_OSX)
# include <CoreAudio/CoreAudio.h>
#endif
#include <AudioUnit/AudioUnit.h>
#include <CoreAudio/CoreAudioTypes.h>

#include <QtCore/QIODevice>
#include <QtCore/QWaitCondition>
#include <QtCore/QMutex>
#include <private/qdarwinaudiodevice_p.h>

QT_BEGIN_NAMESPACE

class QDarwinAudioSinkBuffer;
class QTimer;
class QCoreAudioDeviceInfo;
class CoreAudioRingBuffer;

class QDarwinAudioSinkBuffer : public QObject
{
    Q_OBJECT

public:
    QDarwinAudioSinkBuffer(int bufferSize, int maxPeriodSize, QAudioFormat const& audioFormat);
    ~QDarwinAudioSinkBuffer();

    qint64 readFrames(char *data, qint64 maxFrames);
    qint64 writeBytes(const char *data, qint64 maxSize);

    int available() const;
    void reset();

    void setPrefetchDevice(QIODevice *device);

    void startFillTimer();
    void stopFillTimer();

signals:
    void readyRead();

private slots:
    void fillBuffer();

private:
    bool m_deviceError;
    int m_maxPeriodSize;
    int m_bytesPerFrame;
    int m_periodTime;
    QIODevice *m_device;
    QTimer *m_fillTimer;
    CoreAudioRingBuffer *m_buffer;
};

class QDarwinAudioSinkDevice : public QIODevice
{
public:
    QDarwinAudioSinkDevice(QDarwinAudioSinkBuffer *audioBuffer, QObject *parent);

    qint64 readData(char *data, qint64 len);
    qint64 writeData(const char *data, qint64 len);

    bool isSequential() const { return true; }

private:
    QDarwinAudioSinkBuffer *m_audioBuffer;
};


class QDarwinAudioSink : public QPlatformAudioSink
{
    Q_OBJECT

public:
    QDarwinAudioSink(const QAudioDevice &device);
    ~QDarwinAudioSink();

    void start(QIODevice *device);
    QIODevice *start();
    void stop();
    void reset();
    void suspend();
    void resume();
    qsizetype bytesFree() const;
    void setBufferSize(qsizetype value);
    qsizetype bufferSize() const;
    qint64 processedUSecs() const;
    QAudio::Error error() const;
    QAudio::State state() const;
    void setFormat(const QAudioFormat &format);
    QAudioFormat format() const;

    void setVolume(qreal volume);
    qreal volume() const;

private slots:
    void deviceStopped();
    void inputReady();

private:
    enum {
        Running,
        Draining,
        Stopped
    };

    static OSStatus renderCallback(void *inRefCon,
                                    AudioUnitRenderActionFlags *ioActionFlags,
                                    const AudioTimeStamp *inTimeStamp,
                                    UInt32 inBusNumber,
                                    UInt32 inNumberFrames,
                                    AudioBufferList *ioData);

    bool open();
    void close();
    void audioThreadStart();
    void audioThreadStop();
    void audioThreadDrain();
    void audioDeviceStart();
    void audioDeviceStop();
    void audioDeviceIdle();
    void audioDeviceError();

    void startTimers();
    void stopTimers();

    QAudioDevice m_audioDeviceInfo;
    QByteArray m_device;

    static constexpr int DEFAULT_BUFFER_SIZE = 8 * 1024;

    bool m_isOpen = false;
    int m_internalBufferSize = DEFAULT_BUFFER_SIZE;
    int m_periodSizeBytes = 0;
    qint64 m_totalFrames = 0;
    QAudioFormat m_audioFormat;
    QIODevice *m_audioIO = nullptr;
#if defined(Q_OS_MACOS)
    AudioDeviceID m_audioDeviceId;
#endif
    AudioUnit m_audioUnit = 0;
    Float64 m_clockFrequency = 0;
    AudioStreamBasicDescription m_streamFormat;
    QDarwinAudioSinkBuffer *m_audioBuffer = nullptr;
    QAtomicInt m_audioThreadState;
    QWaitCondition m_threadFinished;
    QMutex m_mutex;
    qreal m_cachedVolume = 1.;
#if defined(Q_OS_MACOS)
    qreal m_volume = 1.;
#endif
    bool m_pullMode = false;

    QAudio::Error m_errorCode = QAudio::NoError;
    QAudio::State m_stateCode = QAudio::StoppedState;
};

QT_END_NAMESPACE

#endif // IOSAUDIOOUTPUT_H
