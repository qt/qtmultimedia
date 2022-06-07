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

#ifndef QAUDIOOUTPUTALSA_H
#define QAUDIOOUTPUTALSA_H

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

class QAlsaAudioSink : public QPlatformAudioSink
{
    friend class AlsaOutputPrivate;
    Q_OBJECT
public:
    QAlsaAudioSink(const QByteArray &device);
    ~QAlsaAudioSink();

    qint64 write( const char *data, qint64 len );

    void start(QIODevice* device) override;
    QIODevice* start() override;
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
    void setFormat(const QAudioFormat& fmt) override;
    QAudioFormat format() const override;
    void setVolume(qreal) override;
    qreal volume() const override;


    QIODevice* audioSource;
    QAudioFormat settings;
    QAudio::Error errorState;
    QAudio::State deviceState;

private slots:
    void userFeed();
    bool deviceReady();

signals:
    void processMore();

private:
    bool opened;
    bool pullMode;
    bool resuming;
    int buffer_size;
    int period_size;
    qint64 totalTimeValue;
    unsigned int buffer_time;
    unsigned int period_time;
    snd_pcm_uframes_t buffer_frames;
    snd_pcm_uframes_t period_frames;
    int xrun_recovery(int err);

    int setFormat();
    bool open();
    void close();

    QTimer* timer;
    QByteArray m_device;
    int bytesAvailable;
    qint64 elapsedTimeOffset;
    char* audioBuffer;
    snd_pcm_t* handle;
    snd_pcm_access_t access;
    snd_pcm_format_t pcmformat;
    snd_pcm_hw_params_t *hwparams;
    qreal m_volume;
};

class AlsaOutputPrivate : public QIODevice
{
    friend class QAlsaAudioSink;
    Q_OBJECT
public:
    AlsaOutputPrivate(QAlsaAudioSink* audio);
    ~AlsaOutputPrivate();

    qint64 readData( char* data, qint64 len) override;
    qint64 writeData(const char* data, qint64 len) override;

private:
    QAlsaAudioSink *audioDevice;
};

QT_END_NAMESPACE


#endif
