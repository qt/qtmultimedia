// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWASMAUDIOSOURCE_H
#define QWASMAUDIOSOURCE_H

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

QT_BEGIN_NAMESPACE

class ALData;

class QWasmAudioSource : public QPlatformAudioSource
{
    Q_OBJECT

    QByteArray m_name;
    ALData *aldata = nullptr;
    QTimer *m_timer = nullptr;
    QIODevice *m_device = nullptr;
    QAudioFormat m_format;
    qreal m_volume = 1;
    int m_bufferSize;
    bool m_running = false;
    bool m_suspended = false;
    QAudio::Error m_error;
    bool m_pullMode;
    char *m_tmpData = nullptr;
    QElapsedTimer m_elapsedTimer;
    int m_notifyInterval = 0;
    quint64 m_processed = 0;

    void writeBuffer();
public:
    QWasmAudioSource(const QByteArray &device, QObject *parent);

public:
    void start(QIODevice *device) override;
    QIODevice *start() override;
    void start(bool mode);
    void stop() override;
    void reset() override;
    void suspend() override;
    void resume() override;
    int bytesReady() const override;
    void setBufferSize(int value) override;
    int bufferSize() const override;
    qint64 processedUSecs() const override;
    QAudio::Error error() const override;
    QAudio::State state() const override;
    void setFormat(const QAudioFormat &fmt) override;
    QAudioFormat format() const override;
    void setVolume(qreal volume) override;
    qreal volume() const override;

    friend class QWasmAudioSourceDevice;
    void setError(const QAudio::Error &error);
};

QT_END_NAMESPACE

#endif // QWASMAUDIOSOURCE_H
