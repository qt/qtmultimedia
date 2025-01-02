// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWASMAUDIOSINK_H
#define QWASMAUDIOSINK_H

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
#include <QTimer>
#include <QElapsedTimer>

class ALData;
class QIODevice;

QT_BEGIN_NAMESPACE

class QWasmAudioSink : public QPlatformAudioSink
{
    Q_OBJECT

    QByteArray m_name;
    ALData *aldata = nullptr;
    QTimer *m_timer = nullptr;
    QIODevice *m_device = nullptr;
    QAudioFormat m_format;
    QAudio::Error m_error = QAudio::NoError;
    bool m_running = false;
    QAudio::State m_state = QAudio::StoppedState;
    QAudio::State m_suspendedInState = QAudio::SuspendedState;
    int m_bufferSize = 0;
    quint64 m_processed = 0;
    QElapsedTimer m_elapsedTimer;
    int m_bufferFragmentsCount = 10;
    int m_notifyInterval = 0;
    char *m_tmpData = nullptr;
    int m_bufferFragmentSize = 0;
    int m_lastNotified = 0;
    int m_tmpDataOffset = 0;
    int m_bufferFragmentsBusyCount = 0;
    bool m_pullMode;
    qreal m_volume = 1;

    void loadALBuffers();
    void unloadALBuffers();
    void nextALBuffers();

private slots:
    void updateState();
    void setError(QAudio::Error);

public:
    QWasmAudioSink(const QByteArray &device, QObject *parent);
    ~QWasmAudioSink();

public:
    void start(QIODevice *device) override;
    QIODevice *start() override;
    void start(bool mode);
    void stop() override;
    void reset() override;
    void suspend() override;
    void resume() override;
    int bytesFree() const override;
    void setBufferSize(int value) override;
    int bufferSize() const override;
    qint64 processedUSecs() const override;
    QAudio::Error error() const override;
    QAudio::State state() const override;
    void setFormat(const QAudioFormat &fmt) override;
    QAudioFormat format() const override;
    void setVolume(qreal volume) override;
    qreal volume() const override;

    friend class QWasmAudioSinkDevice;
};

QT_END_NAMESPACE

#endif // QWASMAUDIOSINK_H
