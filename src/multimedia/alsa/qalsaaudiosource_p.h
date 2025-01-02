// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//


#ifndef QAUDIOINPUTALSA_H
#define QAUDIOINPUTALSA_H

#include <alsa/asoundlib.h>

#include <QtCore/qfile.h>
#include <QtCore/qdebug.h>
#include <QtCore/qtimer.h>
#include <QtCore/qstring.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qelapsedtimer.h>
#include <QtCore/qiodevice.h>

#include <QtMultimedia/qaudio.h>
#include <QtMultimedia/qaudiodevice.h>
#include <private/qaudiosystem_p.h>

QT_BEGIN_NAMESPACE


class AlsaInputPrivate;

class RingBuffer
{
public:
    RingBuffer();

    void resize(int size);

    int bytesOfDataInBuffer() const;
    int freeBytes() const;

    const char *availableData() const;
    int availableDataBlockSize() const;
    void readBytes(int bytes);

    void write(char *data, int len);

private:
    int m_head;
    int m_tail;

    QByteArray m_data;
};

class QAlsaAudioSource : public QPlatformAudioSource
{
    Q_OBJECT
public:
    QAlsaAudioSource(const QByteArray &device, QObject *parent);
    ~QAlsaAudioSource();

    qint64 read(char* data, qint64 len);

    void start(QIODevice* device) override;
    QIODevice* start() override;
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
    void setFormat(const QAudioFormat& fmt) override;
    QAudioFormat format() const override;
    void setVolume(qreal) override;
    qreal volume() const override;
    bool resuming;
    snd_pcm_t* handle;
    qint64 totalTimeValue;
    QIODevice* audioSource;
    QAudioFormat settings;
    QAudio::Error errorState;
    QAudio::State deviceState;

private slots:
    void userFeed();
    bool deviceReady();

private:
    int checkBytesReady();
    int xrun_recovery(int err);
    int setFormat();
    bool open();
    void close();
    void drain();

    QTimer* timer;
    qint64 elapsedTimeOffset;
    RingBuffer ringBuffer;
    qsizetype bytesAvailable;
    QByteArray m_device;
    bool pullMode;
    qsizetype buffer_size;
    int period_size;
    unsigned int buffer_time;
    unsigned int period_time;
    snd_pcm_uframes_t buffer_frames;
    snd_pcm_uframes_t period_frames;
    snd_pcm_access_t access;
    snd_pcm_format_t pcmformat;
    snd_pcm_hw_params_t *hwparams;
    qreal m_volume;
};

class AlsaInputPrivate : public QIODevice
{
    Q_OBJECT
public:
    AlsaInputPrivate(QAlsaAudioSource* audio);
    ~AlsaInputPrivate();

    qint64 readData( char* data, qint64 len) override;
    qint64 writeData(const char* data, qint64 len) override;

    void trigger();
private:
    QAlsaAudioSource *audioDevice;
};

QT_END_NAMESPACE


#endif
