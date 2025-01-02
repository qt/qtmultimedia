// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef IOSAUDIOINPUT_H
#define IOSAUDIOINPUT_H

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

#include <private/qaudiosystem_p.h>
#include <qdarwinaudiodevice_p.h>

#include <AudioUnit/AudioUnit.h>
#include <CoreAudio/CoreAudioTypes.h>
#include <AudioToolbox/AudioToolbox.h>

#include <QtCore/QIODevice>
#include <QtCore/QWaitCondition>
#include <QtCore/QMutex>
#include <QtCore/QTimer>

QT_BEGIN_NAMESPACE

class CoreAudioRingBuffer;
class QCoreAudioPacketFeeder;
class QDarwinAudioSourceBuffer;
class QDarwinAudioSourceDevice;

class QCoreAudioBufferList
{
public:
    QCoreAudioBufferList(AudioStreamBasicDescription const& streamFormat);
    QCoreAudioBufferList(AudioStreamBasicDescription const& streamFormat, char *buffer, int bufferSize);
    QCoreAudioBufferList(AudioStreamBasicDescription const& streamFormat, int framesToBuffer);

    ~QCoreAudioBufferList();

    AudioBufferList* audioBufferList() const { return m_bufferList; }
    char *data(int buffer = 0) const;
    qint64 bufferSize(int buffer = 0) const;
    int frameCount(int buffer = 0) const;
    int packetCount(int buffer = 0) const;
    int packetSize() const;
    void reset();

private:
    bool m_owner;
    int m_dataSize;
    AudioStreamBasicDescription m_streamDescription;
    AudioBufferList *m_bufferList;
};

class QCoreAudioPacketFeeder
{
public:
    QCoreAudioPacketFeeder(QCoreAudioBufferList *abl);

    bool feed(AudioBufferList& dst, UInt32& packetCount);
    bool empty() const;

private:
    UInt32 m_totalPackets;
    UInt32 m_position;
    QCoreAudioBufferList *m_audioBufferList;
};

class QDarwinAudioSourceBuffer : public QObject
{
    Q_OBJECT

public:
    QDarwinAudioSourceBuffer(int bufferSize,
                        int maxPeriodSize,
                        AudioStreamBasicDescription const& inputFormat,
                        AudioStreamBasicDescription const& outputFormat,
                        QObject *parent);

    ~QDarwinAudioSourceBuffer();

    qreal volume() const;
    void setVolume(qreal v);

    qint64 renderFromDevice(AudioUnit audioUnit,
                             AudioUnitRenderActionFlags *ioActionFlags,
                             const AudioTimeStamp *inTimeStamp,
                             UInt32 inBusNumber,
                             UInt32 inNumberFrames);

    qint64 readBytes(char *data, qint64 len);

    void setFlushDevice(QIODevice *device);

    void startFlushTimer();
    void stopFlushTimer();

    void flush(bool all = false);
    void reset();
    int available() const;
    int used() const;

    void lock() { m_mutex.lock(); }
    void unlock() { m_mutex.unlock(); }

    void wait() { m_threadFinished.wait(&m_mutex); }
    void wake() { m_threadFinished.wakeOne(); }

signals:
    void readyRead();

private slots:
    void flushBuffer();

private:
    QMutex m_mutex;
    QWaitCondition m_threadFinished;

    bool m_deviceError;
    int m_maxPeriodSize;
    int m_periodTime;
    QIODevice *m_device;
    QTimer *m_flushTimer;
    CoreAudioRingBuffer *m_buffer;
    QCoreAudioBufferList *m_inputBufferList;
    AudioConverterRef m_audioConverter;
    AudioStreamBasicDescription m_inputFormat;
    AudioStreamBasicDescription m_outputFormat;
    QAudioFormat m_qFormat;
    qreal m_volume;

    const static OSStatus as_empty = 'qtem';

    // Converter callback
    static OSStatus converterCallback(AudioConverterRef inAudioConverter,
                                UInt32 *ioNumberDataPackets,
                                AudioBufferList *ioData,
                                AudioStreamPacketDescription **outDataPacketDescription,
                                void *inUserData);
};

class QDarwinAudioSourceDevice : public QIODevice
{
    Q_OBJECT

public:
    QDarwinAudioSourceDevice(QDarwinAudioSourceBuffer *audioBuffer, QObject *parent);

    qint64 readData(char *data, qint64 len);
    qint64 writeData(const char *data, qint64 len);

    bool isSequential() const { return true; }

private:
    QDarwinAudioSourceBuffer *m_audioBuffer;
};

class QDarwinAudioSource : public QPlatformAudioSource
{
    Q_OBJECT

public:
    QDarwinAudioSource(const QAudioDevice &device, QObject *parent);
    ~QDarwinAudioSource();

    void start(QIODevice *device);
    QIODevice *start();
    void stop();
    void reset();
    void suspend();
    void resume();
    qsizetype bytesReady() const;
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
    void deviceStoppped();

private:
    enum {
        Running,
        Stopped
    };

    bool open();
    void close();

    void audioThreadStart();
    void audioThreadStop();

    void audioDeviceStop();
    void audioDeviceActive();
    void audioDeviceFull();
    void audioDeviceError();

    void startTimers();
    void stopTimers();

    // Input callback
    static OSStatus inputCallback(void *inRefCon,
                                    AudioUnitRenderActionFlags *ioActionFlags,
                                    const AudioTimeStamp *inTimeStamp,
                                    UInt32 inBusNumber,
                                    UInt32 inNumberFrames,
                                    AudioBufferList *ioData);

    QAudioDevice m_audioDeviceInfo;
    QByteArray m_device;
    bool m_isOpen;
    int m_periodSizeBytes;
    int m_internalBufferSize;
    qint64 m_totalFrames;
    QAudioFormat m_audioFormat;
    QIODevice *m_audioIO;
    AudioUnit m_audioUnit;
#if defined(Q_OS_MACOS)
    AudioDeviceID m_audioDeviceId;
#endif
    Float64 m_clockFrequency;
    QAudio::Error m_errorCode;
    QAudio::State m_stateCode;
    QDarwinAudioSourceBuffer *m_audioBuffer;
    QAtomicInt m_audioThreadState;
    AudioStreamBasicDescription m_streamFormat;
    AudioStreamBasicDescription m_deviceFormat;
    qreal m_volume;
};

QT_END_NAMESPACE

#endif // IOSAUDIOINPUT_H
