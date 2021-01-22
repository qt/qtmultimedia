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

#ifndef QAUDIOOUTPUTPULSE_H
#define QAUDIOOUTPUTPULSE_H

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

#include <QtCore/qfile.h>
#include <QtCore/qtimer.h>
#include <QtCore/qstring.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qelapsedtimer.h>
#include <QtCore/qiodevice.h>

#include "qaudio.h"
#include "qaudiodeviceinfo.h"
#include "qaudiosystem.h"

#include <pulse/pulseaudio.h>

QT_BEGIN_NAMESPACE

class QPulseAudioOutput : public QAbstractAudioOutput
{
    friend class PulseOutputPrivate;
    Q_OBJECT

public:
    QPulseAudioOutput(const QByteArray &device);
    ~QPulseAudioOutput();

    void start(QIODevice *device) override;
    QIODevice *start() override;
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
    void setFormat(const QAudioFormat &format) override;
    QAudioFormat format() const override;

    void setVolume(qreal volume) override;
    qreal volume() const override;

    void setCategory(const QString &category) override;
    QString category() const override;

public:
    void streamUnderflowCallback();

private:
    void setState(QAudio::State state);
    void setError(QAudio::Error error);

    bool open();
    void close();
    qint64 write(const char *data, qint64 len);

private Q_SLOTS:
    void userFeed();
    void onPulseContextFailed();

private:
    QByteArray m_device;
    QByteArray m_streamName;
    QAudioFormat m_format;
    QAudio::Error m_errorState;
    QAudio::State m_deviceState;
    bool m_pullMode;
    bool m_opened;
    QIODevice *m_audioSource;
    QTimer m_periodTimer;
    int m_periodTime;
    pa_stream *m_stream;
    int m_notifyInterval;
    int m_periodSize;
    int m_bufferSize;
    int m_maxBufferSize;
    QElapsedTimer m_clockStamp;
    qint64 m_totalTimeValue;
    QTimer *m_tickTimer;
    char *m_audioBuffer;
    QElapsedTimer m_timeStamp;
    qint64 m_elapsedTimeOffset;
    bool m_resuming;
    QString m_category;

    qreal m_volume;
    pa_sample_spec m_spec;
};

class PulseOutputPrivate : public QIODevice
{
    friend class QPulseAudioOutput;
    Q_OBJECT

public:
    PulseOutputPrivate(QPulseAudioOutput *audio);
    virtual ~PulseOutputPrivate() {}

protected:
    qint64 readData(char *data, qint64 len) override;
    qint64 writeData(const char *data, qint64 len) override;

private:
    QPulseAudioOutput *m_audioDevice;
};

QT_END_NAMESPACE

#endif
