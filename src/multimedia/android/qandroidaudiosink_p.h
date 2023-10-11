// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOPENSLESAUDIOOUTPUT_H
#define QOPENSLESAUDIOOUTPUT_H

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
#include <SLES/OpenSLES.h>
#include <qbytearray.h>
#include <qmap.h>
#include <QElapsedTimer>
#include <QIODevice>

QT_BEGIN_NAMESPACE

class QAndroidAudioSink : public QPlatformAudioSink
{
    Q_OBJECT

public:
    QAndroidAudioSink(const QByteArray &device, QObject *parent);
    ~QAndroidAudioSink();

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
    void setFormat(const QAudioFormat &format) override;
    QAudioFormat format() const override;

    void setVolume(qreal volume) override;
    qreal volume() const override;

private:
    friend class SLIODevicePrivate;

    Q_INVOKABLE void onEOSEvent();
    Q_INVOKABLE void onBytesProcessed(qint64 bytes);
    Q_INVOKABLE void bufferAvailable();

    static void playCallback(SLPlayItf playItf, void *ctx, SLuint32 event);
    static void bufferQueueCallback(SLBufferQueueItf bufferQueue, void *ctx);

    bool preparePlayer();
    void destroyPlayer();
    void stopPlayer();
    void readyRead();
    void startPlayer();
    qint64 writeData(const char *data, qint64 len);

    void setState(QAudio::State state);
    void setError(QAudio::Error error);

    SLmillibel adjustVolume(qreal vol);

    static constexpr int BufferCount = 2;

    QByteArray m_deviceName;
    QAudio::State m_state = QAudio::StoppedState;
    QAudio::State m_suspendedInState = QAudio::SuspendedState;
    QAudio::Error m_error = QAudio::NoError;
    SLObjectItf m_outputMixObject = nullptr;
    SLObjectItf m_playerObject = nullptr;
    SLPlayItf m_playItf = nullptr;
    SLVolumeItf m_volumeItf = nullptr;
    SLBufferQueueItf m_bufferQueueItf = nullptr;
    QIODevice *m_audioSource = nullptr;
    char *m_buffers = nullptr;
    qreal m_volume = 1.0;
    bool m_pullMode = false;
    int m_nextBuffer = 0;
    int m_bufferSize = 0;
    qint64 m_elapsedTime = 0;
    qint64 m_processedBytes = 0;
    bool m_endSound = false;
    QAtomicInt m_availableBuffers = BufferCount;
    SLuint32 m_eventMask = SL_PLAYEVENT_HEADATEND;
    bool m_startRequiresInit = true;

    qint32 m_streamType;
    QAudioFormat m_format;
};

class SLIODevicePrivate : public QIODevice
{
    Q_OBJECT

public:
    inline SLIODevicePrivate(QAndroidAudioSink *audio) : m_audioDevice(audio) {}
    inline ~SLIODevicePrivate() override {}

protected:
    inline qint64 readData(char *, qint64) override { return 0; }
    inline qint64 writeData(const char *data, qint64 len) override;

private:
    QAndroidAudioSink *m_audioDevice;
};

qint64 SLIODevicePrivate::writeData(const char *data, qint64 len)
{
    Q_ASSERT(m_audioDevice);
    return m_audioDevice->writeData(data, len);
}

QT_END_NAMESPACE

#endif // QOPENSLESAUDIOOUTPUT_H
