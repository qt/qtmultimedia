// Copyright (C) 2016 Research In Motion
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QNXAUDIOOUTPUT_H
#define QNXAUDIOOUTPUT_H

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

#include "private/qaudiosystem_p.h"

#include "qqnxaudioutils_p.h"

#include <QElapsedTimer>
#include <QTimer>
#include <QIODevice>
#include <QSocketNotifier>

#include <sys/asoundlib.h>
#include <sys/neutrino.h>

QT_BEGIN_NAMESPACE

class QnxPushIODevice;

class QQnxAudioSink : public QPlatformAudioSink
{
    Q_OBJECT

public:
    explicit QQnxAudioSink(const QAudioDevice &deviceInfo, QObject *parent);
    ~QQnxAudioSink();

    void start(QIODevice *source) override;
    QIODevice *start() override;
    void stop() override;
    void reset() override;
    void suspend() override;
    void resume() override;
    qsizetype bytesFree() const override;
    void setBufferSize(qsizetype) override;
    qsizetype bufferSize() const override;
    qint64 processedUSecs() const override;
    QAudio::Error error() const override;
    QAudio::State state() const override;
    void setFormat(const QAudioFormat &format) override;
    QAudioFormat format() const override;
    void setVolume(qreal volume) override;
    qreal volume() const override;
    qint64 pushData(const char *data, qint64 len);

private slots:
    void pullData();
    void pcmNotifierActivated(int socket);

private:
    bool open();
    void close();
    void changeState(QAudio::State state, QAudio::Error error);

    void addPcmEventFilter();
    void createPcmNotifiers();
    void destroyPcmNotifiers();

    void suspendInternal(QAudio::State suspendState);
    void resumeInternal();

    void updateState();

    qint64 write(const char *data, qint64 len);

    QIODevice *m_source;
    bool m_pushSource;
    QTimer *m_timer;

    QAudio::Error m_error;
    QAudio::State m_state;
    QAudio::State m_suspendedInState;
    QAudioFormat m_format;
    qreal m_volume;
    int m_periodSize;

    QnxAudioUtils::HandleUniquePtr m_pcmHandle;
    qint64 m_bytesWritten;

    int m_requestedBufferSize;

    QAudioDevice m_deviceInfo;

    QSocketNotifier *m_pcmNotifier;
};

class QnxPushIODevice : public QIODevice
{
    Q_OBJECT
public:
    explicit QnxPushIODevice(QQnxAudioSink *output);
    ~QnxPushIODevice();

    qint64 readData(char *data, qint64 len);
    qint64 writeData(const char *data, qint64 len);

    bool isSequential() const override;

private:
    QQnxAudioSink *m_output;
};

QT_END_NAMESPACE

#endif
