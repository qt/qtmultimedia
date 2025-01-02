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
#include <qcomptr_p.h>
#include <qwindowsresampler_p.h>

#include <audioclient.h>
#include <mmdeviceapi.h>

QT_BEGIN_NAMESPACE

class QWindowsResampler;

class QWindowsAudioSink : public QPlatformAudioSink
{
    Q_OBJECT
public:
    QWindowsAudioSink(ComPtr<IMMDevice> device, QObject *parent);
    ~QWindowsAudioSink();

    void setFormat(const QAudioFormat& fmt) override;
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
    qint64 write(const char *data, qint64 len);
    qint64 push(const char *data, qint64 len);

    bool open();
    void close();

    void deviceStateChange(QAudio::State, QAudio::Error);

    void pullSource();
    qint64 remainingPlayTimeUs();

    QAudioFormat m_format;
    QAudio::Error errorState = QAudio::NoError;
    QAudio::State deviceState = QAudio::StoppedState;
    QAudio::State suspendedInState = QAudio::SuspendedState;

    qsizetype m_bufferSize = 0;
    qreal m_volume = 1.0;
    QTimer *m_timer = nullptr;
    QScopedPointer<QIODevice> m_pushSource;
    QPointer<QIODevice> m_pullSource;
    ComPtr<IMMDevice> m_device;
    ComPtr<IAudioClient> m_audioClient;
    ComPtr<IAudioRenderClient> m_renderClient;
    QWindowsResampler m_resampler;
};

QT_END_NAMESPACE


#endif // QWINDOWSAUDIOOUTPUT_H
