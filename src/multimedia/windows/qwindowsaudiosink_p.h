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

#ifndef QWINDOWSAUDIOOUTPUT_H
#define QWINDOWSAUDIOOUTPUT_H

#include <QtCore/qdebug.h>
#include <QtCore/qelapsedtimer.h>
#include <QtCore/qiodevice.h>
#include <QtCore/qstring.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qmutex.h>
#include <QtCore/qtimer.h>
#include <QtCore/qpointer.h>

#include <QtMultimedia/qaudio.h>
#include <QtMultimedia/qaudiodevice.h>
#include <private/qaudiosystem_p.h>
#include <QtCore/private/qcomptr_p.h>
#include <qwindowsresampler_p.h>

#include <audioclient.h>
#include <mmdeviceapi.h>
#include <chrono>

QT_BEGIN_NAMESPACE

class QWindowsResampler;

class AudioClient
{
public:
    static std::unique_ptr<AudioClient> create(const ComPtr<IMMDevice> &device,
                                               const QAudioFormat &format, qsizetype &bufferSize);
    std::chrono::microseconds remainingPlayTime();
    quint64 bytesFree() const;
    quint64 totalInputBytes() const;
    qint64 render(const QAudioFormat &format, qreal volume, const char *data, qint64 len);
    void start();
    void stop();
    bool resetResampler();

private:
    AudioClient(const ComPtr<IMMDevice> &device, const QAudioFormat &format);
    bool create(qsizetype& bufferSize);
    std::optional<quint32> availableFrameCount() const;

    ComPtr<IMMDevice> m_device;
    ComPtr<IAudioClient> m_audioClient;
    ComPtr<IAudioRenderClient> m_renderClient;
    QWindowsResampler m_resampler;
    QAudioFormat m_inputFormat;
    QAudioFormat m_outputFormat;
};

class QWindowsAudioSink : public QPlatformAudioSink
{
    Q_OBJECT
public:
    QWindowsAudioSink(ComPtr<IMMDevice> device, const QAudioFormat &fmt, QObject *parent);
    ~QWindowsAudioSink();

    QAudioFormat format() const override;
    QIODevice* start() override;
    void start(QIODevice* device) override;
    void stop() override;
    void reset() override;
    void suspend() override;
    void resume() override;
    qsizetype bytesFree() const override;
    void setBufferSize(qsizetype value) override;
    qsizetype bufferSize() const override { return m_bufferSize; }
    qint64 processedUSecs() const override;
    QAudio::Error error() const override { return errorState; }
    QAudio::State state() const override { return deviceState; }
    void setVolume(qreal) override;
    qreal volume() const override { return m_volume; }

private:
    friend class OutputPrivate;
    qint64 push(const char *data, qint64 len);

    bool open();
    void close();
    void deviceStateChange(QAudio::State, QAudio::Error);

    void pullSource();

    const QAudioFormat m_format;
    QAudio::Error errorState = QAudio::NoError;
    QAudio::State deviceState = QAudio::StoppedState;
    QAudio::State suspendedInState = QAudio::SuspendedState;

    qsizetype m_bufferSize = 0;
    qreal m_volume = 1.0;
    QTimer *m_timer = nullptr;
    QScopedPointer<QIODevice> m_pushSource;
    QPointer<QIODevice> m_pullSource;
    ComPtr<IMMDevice> m_device;
    bool m_recreateClient = true;
    std::unique_ptr<AudioClient> m_client;
};

QT_END_NAMESPACE


#endif // QWINDOWSAUDIOOUTPUT_H
