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

#include <QtCore/qiodevice.h>
#include <QtCore/qtimer.h>

#include <QtMultimedia/private/qaudioringbuffer_p.h>
#include <QtMultimedia/private/qaudiostatemachine_p.h>
#include <QtMultimedia/private/qaudiosystem_p.h>
#include <QtMultimedia/private/qcoreaudioutils_p.h>
#include <QtMultimedia/private/qdarwinaudiodevice_p.h>
#include <QtMultimedia/private/qdarwinaudiounit_p.h>

#include <AudioUnit/AudioUnit.h>
#include <CoreAudio/CoreAudioTypes.h>
#include <AudioToolbox/AudioConverter.h>


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
    bool m_owner = false;
    int m_dataSize = 0;
    AudioStreamBasicDescription m_streamDescription;
    AudioBufferList *m_bufferList = nullptr;
};

class QCoreAudioPacketFeeder
{
public:
    QCoreAudioPacketFeeder(QCoreAudioBufferList *abl);

    bool feed(AudioBufferList& dst, UInt32& packetCount);
    bool empty() const;

private:
    UInt32 m_totalPackets;
    UInt32 m_position = 0;
    QCoreAudioBufferList *m_audioBufferList;
};

class QDarwinAudioSource;

class QDarwinAudioSourceBuffer : public QObject
{
    Q_OBJECT

public:
    QDarwinAudioSourceBuffer(const QDarwinAudioSource &audioSource,
                             int bufferSize,
                             int maxPeriodSize,
                             const AudioStreamBasicDescription &inputFormat,
                             const AudioStreamBasicDescription &outputFormat,
                             QObject *parent);

    qint64 renderFromDevice(AudioUnit audioUnit,
                            AudioUnitRenderActionFlags *ioActionFlags,
                            const AudioTimeStamp *inTimeStamp,
                            UInt32 inBusNumber,
                            UInt32 inNumberFrames);

    qint64 readBytes(char *data, qint64 len);

    void setFlushDevice(QIODevice *device);

    void setFlushingEnabled(bool enabled);

    void reset();
    void flushAll() { flush(true); }
    int available() const;
    int used() const;

private:
    void flush(bool all = false);

signals:
    void readyRead();

private slots:
    void flushBuffer();

private:
    const QDarwinAudioSource &m_audioSource;
    bool m_deviceError = false;
    bool m_flushingEnabled = false;
    int m_maxPeriodSize = 0;
    QIODevice *m_device = nullptr;
    QTimer *m_flushTimer = nullptr;
    QtPrivate::QAudioRingBuffer<char> m_buffer;
    QCoreAudioBufferList m_inputBufferList;
    AudioConverterRef m_audioConverter = nullptr;
    const AudioStreamBasicDescription m_outputFormat;
    QAudioFormat m_qFormat;

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

    qint64 readData(char *data, qint64 len) override;
    qint64 writeData(const char *data, qint64 len) override;

    bool isSequential() const override { return true; }

private:
    QDarwinAudioSourceBuffer *m_audioBuffer;
};

class QDarwinAudioSource : public QPlatformAudioSource
{
    Q_OBJECT

public:
    QDarwinAudioSource(const QAudioDevice &device, const QAudioFormat &format, QObject *parent);
    ~QDarwinAudioSource() override;

    void start(QIODevice *device) override;
    QIODevice *start() override;
    void stop() override;
    void reset() override;
    void suspend() override;
    void resume() override;
    qsizetype bytesReady() const override;
    void setBufferSize(qsizetype value) override;
    qsizetype bufferSize() const override;
    qint64 processedUSecs() const override;
    QAudio::Error error() const override;
    QAudio::State state() const override;
    QAudioFormat format() const override;

    void setVolume(qreal volume) override;
    qreal volume() const override;

    bool audioUnitStarted() const { return m_audioUnitState == AudioUnitState::Started; }

private slots:
    void appStateChanged(Qt::ApplicationState state);

private:
    bool open();
    void close();

    void onAudioDeviceError();
    void onAudioDeviceFull();
    void onAudioDeviceActive();

    void updateAudioDevice();

    // Input callback
    static OSStatus inputCallback(void *inRefCon,
                                  AudioUnitRenderActionFlags *ioActionFlags,
                                  const AudioTimeStamp *inTimeStamp,
                                  UInt32 inBusNumber,
                                  UInt32 inNumberFrames,
                                  AudioBufferList *ioData);

    QAudioDevice m_audioDevice;
    bool m_isOpen = false;
    int m_periodSizeBytes = 0;
    int m_internalBufferSize = 0;
    qint64 m_totalFrames = 0;
    const QAudioFormat m_audioFormat;
    QIODevice *m_audioIO = nullptr;
    AudioUnit m_audioUnit = 0;
    std::unique_ptr<QDarwinAudioSourceBuffer> m_audioBuffer;
    AudioStreamBasicDescription m_streamFormat;
    AudioStreamBasicDescription m_deviceFormat;
    qreal m_volume = qreal(1.0);

#if defined(Q_OS_MACOS)
    bool addDisconnectListener(AudioObjectID);
    void removeDisconnectListener();

    QCoreAudioUtils::DeviceDisconnectMonitor m_disconnectMonitor;
    QFuture<void> m_stopOnDisconnected;
#endif

    AudioUnitState m_audioUnitState = AudioUnitState::Stopped;
    QAudioStateMachine m_stateMachine;
};

QT_END_NAMESPACE

#endif // IOSAUDIOINPUT_H
