// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
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

#if defined(Q_OS_MACOS)
#  include <CoreAudio/CoreAudio.h>
#endif
#include <AudioUnit/AudioUnit.h>
#include <CoreAudio/CoreAudioTypes.h>

#include <QtCore/qiodevice.h>
#include <QtCore/qsemaphore.h>
#include <QtMultimedia/private/qaudioringbuffer_p.h>
#include <QtMultimedia/private/qaudiostatemachine_p.h>
#include <QtMultimedia/private/qaudiosystem_p.h>
#include <QtMultimedia/private/qcoreaudioutils_p.h>
#include <QtMultimedia/private/qdarwinaudiodevice_p.h>
#include <QtMultimedia/private/qdarwinaudiounit_p.h>

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
    ~QDarwinAudioSinkBuffer() override;

    qint64 readFrames(char *data, qint64 maxFrames);
    qint64 writeBytes(const char *data, qint64 maxSize);

    int available() const;

    bool deviceAtEnd() const;

    void reset();

    void setPrefetchDevice(QIODevice *device);

    QIODevice *prefetchDevice() const;

    void setFillingEnabled(bool enabled);

signals:
    void readyRead();

private slots:
    void fillBuffer();

private:
    bool m_deviceError = false;
    bool m_fillingEnabled = false;
    bool m_deviceAtEnd = false;
    const int m_maxPeriodSize = 0;
    const int m_bytesPerFrame = 0;
    const int m_periodTime = 0;
    QIODevice *m_device = nullptr;
    QTimer *m_fillTimer = nullptr;
    QtPrivate::QAudioRingBuffer<char> m_buffer;
};

class QDarwinAudioSinkDevice : public QIODevice
{
public:
    QDarwinAudioSinkDevice(QDarwinAudioSinkBuffer *audioBuffer, QObject *parent);

    qint64 readData(char *data, qint64 len) override;
    qint64 writeData(const char *data, qint64 len) override;

    bool isSequential() const override { return true; }

private:
    QDarwinAudioSinkBuffer *m_audioBuffer;
};


class QDarwinAudioSink : public QPlatformAudioSink
{
    Q_OBJECT

public:
    QDarwinAudioSink(const QAudioDevice &device, const QAudioFormat &format, QObject *parent);
    ~QDarwinAudioSink() override;

    void start(QIODevice *device) override;
    QIODevice *start() override;
    void stop() override;
    void reset() override;
    void suspend() override;
    void resume() override;
    qsizetype bytesFree() const override;
    void setBufferSize(qsizetype value) override;
    qsizetype bufferSize() const override;
    qint64 processedUSecs() const override;
    QAudio::Error error() const override;
    QAudio::State state() const override;
    QAudioFormat format() const override;

    void setVolume(qreal volume) override;
    qreal volume() const override;

private slots:
    void inputReady();
    void updateAudioDevice();
    void appStateChanged(Qt::ApplicationState state);

private:
    enum ThreadState { Running, Draining, Stopped };

    static OSStatus renderCallback(void *inRefCon,
                                    AudioUnitRenderActionFlags *ioActionFlags,
                                    const AudioTimeStamp *inTimeStamp,
                                    UInt32 inBusNumber,
                                    UInt32 inNumberFrames,
                                    AudioBufferList *ioData);

    bool open();
    void close();
    void onAudioDeviceIdle();
    void onAudioDeviceError();
    void onAudioDeviceDrained();

    QAudioDevice m_audioDevice;

    static constexpr int DEFAULT_BUFFER_SIZE = 8 * 1024;

    bool m_isOpen = false;
    int m_internalBufferSize = DEFAULT_BUFFER_SIZE;
    int m_periodSizeBytes = 0;
    qint64 m_totalFrames = 0;
    const QAudioFormat m_audioFormat;
    QIODevice *m_audioIO = nullptr;
    AudioUnit m_audioUnit = 0;
    AudioStreamBasicDescription m_streamFormat;
    std::unique_ptr<QDarwinAudioSinkBuffer> m_audioBuffer;
    qreal m_cachedVolume = 1.;
#if defined(Q_OS_MACOS)
    bool addDisconnectListener(AudioObjectID);
    void removeDisconnectListener();

    QCoreAudioUtils::DeviceDisconnectMonitor m_disconnectMonitor;
    QFuture<void> m_stopOnDisconnected;

    qreal m_volume = 1.;
#endif

    QAudioStateMachine m_stateMachine;
    QSemaphore m_drainSemaphore;
    AudioUnitState m_audioUnitState = AudioUnitState::Stopped;
};

QT_END_NAMESPACE

#endif // IOSAUDIOOUTPUT_H
