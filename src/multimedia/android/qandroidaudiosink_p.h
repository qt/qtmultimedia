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
    QAndroidAudioSink(const QByteArray &device);
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
    void bufferAvailable(quint32 count, quint32 playIndex);

    static void playCallback(SLPlayItf playItf, void *ctx, SLuint32 event);
    static void bufferQueueCallback(SLBufferQueueItf bufferQueue, void *ctx);

    bool preparePlayer();
    void destroyPlayer();
    void stopPlayer();
    void startPlayer();
    qint64 writeData(const char *data, qint64 len);

    void setState(QAudio::State state);
    void setError(QAudio::Error error);

    SLmillibel adjustVolume(qreal vol);

    QByteArray m_deviceName;
    QAudio::State m_state;
    QAudio::Error m_error;
    SLObjectItf m_outputMixObject;
    SLObjectItf m_playerObject;
    SLPlayItf m_playItf;
    SLVolumeItf m_volumeItf;
    SLBufferQueueItf m_bufferQueueItf;
    QIODevice *m_audioSource;
    char *m_buffers;
    qreal m_volume;
    bool m_pullMode;
    int m_nextBuffer;
    int m_bufferSize;
    qint64 m_elapsedTime;
    qint64 m_processedBytes;
    QAtomicInt m_availableBuffers;
    SLuint32 m_eventMask;
    bool m_startRequiresInit;

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
