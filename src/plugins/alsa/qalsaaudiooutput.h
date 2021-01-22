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
#include <QtMultimedia/qaudiodeviceinfo.h>
#include <QtMultimedia/qaudiosystem.h>

QT_BEGIN_NAMESPACE

class QAlsaAudioOutput : public QAbstractAudioOutput
{
    friend class AlsaOutputPrivate;
    Q_OBJECT
public:
    QAlsaAudioOutput(const QByteArray &device);
    ~QAlsaAudioOutput();

    qint64 write( const char *data, qint64 len );

    void start(QIODevice* device) override;
    QIODevice* start() override;
    void stop() override;
    void reset() override;
    void suspend() override;
    void resume() override;
    int bytesFree() const override;
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
    int intervalTime;
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
    QElapsedTimer timeStamp;
    QElapsedTimer clockStamp;
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
    friend class QAlsaAudioOutput;
    Q_OBJECT
public:
    AlsaOutputPrivate(QAlsaAudioOutput* audio);
    ~AlsaOutputPrivate();

    qint64 readData( char* data, qint64 len) override;
    qint64 writeData(const char* data, qint64 len) override;

private:
    QAlsaAudioOutput *audioDevice;
};

QT_END_NAMESPACE


#endif
