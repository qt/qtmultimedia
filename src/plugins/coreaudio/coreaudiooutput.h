/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
****************************************************************************/
#ifndef IOSAUDIOOUTPUT_H
#define IOSAUDIOOUTPUT_H

#include <qaudiosystem.h>

#if defined(Q_OS_OSX)
# include <CoreAudio/CoreAudio.h>
#endif
#include <AudioUnit/AudioUnit.h>
#include <CoreAudio/CoreAudioTypes.h>

#include <QtCore/QIODevice>
#include <QtCore/QWaitCondition>
#include <QtCore/QMutex>

QT_BEGIN_NAMESPACE

class CoreAudioOutputBuffer;
class QTimer;
class CoreAudioDeviceInfo;
class CoreAudioRingBuffer;

class CoreAudioOutputBuffer : public QObject
{
    Q_OBJECT

public:
    CoreAudioOutputBuffer(int bufferSize, int maxPeriodSize, QAudioFormat const& audioFormat);
    ~CoreAudioOutputBuffer();

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

class CoreAudioOutputDevice : public QIODevice
{
public:
    CoreAudioOutputDevice(CoreAudioOutputBuffer *audioBuffer, QObject *parent);

    qint64 readData(char *data, qint64 len);
    qint64 writeData(const char *data, qint64 len);

    bool isSequential() const { return true; }

private:
    CoreAudioOutputBuffer *m_audioBuffer;
};


class CoreAudioOutput : public QAbstractAudioOutput
{
    Q_OBJECT

public:
    CoreAudioOutput(const QByteArray &device);
    ~CoreAudioOutput();

    void start(QIODevice *device);
    QIODevice *start();
    void stop();
    void reset();
    void suspend();
    void resume();
    int bytesFree() const;
    int periodSize() const;
    void setBufferSize(int value);
    int bufferSize() const;
    void setNotifyInterval(int milliSeconds);
    int notifyInterval() const;
    qint64 processedUSecs() const;
    qint64 elapsedUSecs() const;
    QAudio::Error error() const;
    QAudio::State state() const;
    void setFormat(const QAudioFormat &format);
    QAudioFormat format() const;

    void setVolume(qreal volume);
    qreal volume() const;

    void setCategory(const QString &category);
    QString category() const;

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
    void audioDeviceStop();
    void audioDeviceIdle();
    void audioDeviceError();

    void startTimers();
    void stopTimers();

    QByteArray m_device;

    bool m_isOpen;
    int m_internalBufferSize;
    int m_periodSizeBytes;
    qint64 m_totalFrames;
    QAudioFormat m_audioFormat;
    QIODevice *m_audioIO;
#if defined(Q_OS_OSX)
    AudioDeviceID m_audioDeviceId;
#endif
    AudioUnit m_audioUnit;
    Float64 m_clockFrequency;
    UInt64 m_startTime;
    AudioStreamBasicDescription m_streamFormat;
    CoreAudioOutputBuffer *m_audioBuffer;
    QAtomicInt m_audioThreadState;
    QWaitCondition m_threadFinished;
    QMutex m_mutex;
    QTimer *m_intervalTimer;
    CoreAudioDeviceInfo *m_audioDeviceInfo;
    qreal m_cachedVolume;
    qreal m_volume;
    bool m_pullMode;

    QAudio::Error m_errorCode;
    QAudio::State m_stateCode;
};

QT_END_NAMESPACE

#endif // IOSAUDIOOUTPUT_H
