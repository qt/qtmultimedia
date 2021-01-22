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
#include <QtMultimedia/qaudiodeviceinfo.h>
#include <QtMultimedia/qaudiosystem.h>

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

class QAlsaAudioInput : public QAbstractAudioInput
{
    Q_OBJECT
public:
    QAlsaAudioInput(const QByteArray &device);
    ~QAlsaAudioInput();

    qint64 read(char* data, qint64 len);

    void start(QIODevice* device) override;
    QIODevice* start() override;
    void stop() override;
    void reset() override;
    void suspend() override;
    void resume() override;
    int bytesReady() const override;
    int periodSize() const override;
    void setBufferSize(int value) override;
    int bufferSize() const override;
    void setNotifyInterval(int milliSeconds) override;
    int notifyInterval() const override;
    qint64 processedUSecs() const override;
    qint64 elapsedUSecs() const override;
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
    QElapsedTimer timeStamp;
    QElapsedTimer clockStamp;
    qint64 elapsedTimeOffset;
    int intervalTime;
    RingBuffer ringBuffer;
    int bytesAvailable;
    QByteArray m_device;
    bool pullMode;
    int buffer_size;
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
    AlsaInputPrivate(QAlsaAudioInput* audio);
    ~AlsaInputPrivate();

    qint64 readData( char* data, qint64 len) override;
    qint64 writeData(const char* data, qint64 len) override;

    void trigger();
private:
    QAlsaAudioInput *audioDevice;
};

QT_END_NAMESPACE


#endif
