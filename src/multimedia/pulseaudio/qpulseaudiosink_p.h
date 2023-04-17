// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
#include "qaudiodevice.h"
#include "pulseaudio/qpulsehelpers_p.h"

#include <private/qaudiosystem_p.h>
#include <private/qaudiostatemachine_p.h>
#include <pulse/pulseaudio.h>

QT_BEGIN_NAMESPACE

class QPulseAudioSink : public QPlatformAudioSink
{
    friend class PulseOutputPrivate;
    Q_OBJECT

public:
    QPulseAudioSink(const QByteArray &device, QObject *parent);
    ~QPulseAudioSink();

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

    void streamUnderflowCallback();
    void streamDrainedCallback();

protected:
    void timerEvent(QTimerEvent *event) override;

private:
    void startReading();

    bool open();
    void close();
    qint64 write(const char *data, qint64 len);

private Q_SLOTS:
    void userFeed();
    void onPulseContextFailed();

    PAOperationUPtr exchangeDrainOperation(pa_operation *newOperation);

private:
    pa_sample_spec m_spec = {};
    // calculate timing manually, as pulseaudio doesn't give us good enough data
    mutable timeval lastTimingInfo = {};

    mutable QList<qint64> latencyList; // last latency values

    QByteArray m_device;
    QByteArray m_streamName;
    QAudioFormat m_format;
    QBasicTimer m_tickTimer;

    QIODevice *m_audioSource = nullptr;
    pa_stream *m_stream = nullptr;
    std::vector<char> m_audioBuffer;

    qint64 m_totalTimeValue = 0;
    qint64 m_elapsedTimeOffset = 0;
    mutable qint64 averageLatency = 0; // average latency
    mutable qint64 lastProcessedUSecs = 0;
    qreal m_volume = 1.0;

    std::atomic<pa_operation *> m_drainOperation = nullptr;
    int m_periodSize = 0;
    int m_bufferSize = 0;
    int m_periodTime = 0;
    bool m_pullMode = true;
    bool m_opened = false;
    bool m_resuming = false;

    QAudioStateMachine m_stateMachine;
};

class PulseOutputPrivate : public QIODevice
{
    friend class QPulseAudioSink;
    Q_OBJECT

public:
    PulseOutputPrivate(QPulseAudioSink *audio);
    virtual ~PulseOutputPrivate() {}

protected:
    qint64 readData(char *data, qint64 len) override;
    qint64 writeData(const char *data, qint64 len) override;

private:
    QPulseAudioSink *m_audioDevice;
};

QT_END_NAMESPACE

#endif
