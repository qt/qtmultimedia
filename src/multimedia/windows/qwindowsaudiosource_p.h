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

#ifndef QWINDOWSAUDIOINPUT_H
#define QWINDOWSAUDIOINPUT_H

#include "qwindowsaudioutils_p.h"

#include <QtCore/qfile.h>
#include <QtCore/qdebug.h>
#include <QtCore/qelapsedtimer.h>
#include <QtCore/qstring.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qmutex.h>
#include <QtCore/qbytearray.h>

#include <QtMultimedia/qaudio.h>
#include <QtMultimedia/qaudiodevice.h>
#include <private/qaudiosystem_p.h>

#include <qwindowsresampler_p.h>

struct IMMDevice;
struct IAudioClient;
struct IAudioCaptureClient;

QT_BEGIN_NAMESPACE
class QTimer;

class QWindowsAudioSource : public QPlatformAudioSource
{
    Q_OBJECT
public:
    QWindowsAudioSource(ComPtr<IMMDevice> device, const QAudioFormat &fmt, QObject *parent);
    ~QWindowsAudioSource();

    qint64 read(char* data, qint64 len);

    QAudioFormat format() const override;
    QIODevice* start() override;
    void start(QIODevice* device) override;
    void stop() override;
    void reset() override;
    void suspend() override;
    void resume() override;
    qsizetype bytesReady() const override;
    void setBufferSize(qsizetype value) override;
    qsizetype bufferSize() const override;
    qint64 processedUSecs() const override;
    QAudio::Error error() const override;
    QAudio::State state() const override;
    void setVolume(qreal volume) override;
    qreal volume() const override;

private:
    void deviceStateChange(QAudio::State state, QAudio::Error error);
    void pullCaptureClient();
    void schedulePull();
    QByteArray readCaptureClientBuffer();

    QTimer *m_timer = nullptr;
    ComPtr<IMMDevice> m_device;
    ComPtr<IAudioClient> m_audioClient;
    ComPtr<IAudioCaptureClient> m_captureClient;
    QWindowsResampler m_resampler;
    int m_bufferSize = 0;
    qreal m_volume = 1.0;

    QIODevice* m_ourSink = nullptr;
    QIODevice* m_clientSink = nullptr;
    const QAudioFormat m_format;
    QAudio::Error m_errorState = QAudio::NoError;
    QAudio::State m_deviceState = QAudio::StoppedState;

    QByteArray m_clientBufferResidue;

    bool open();
    void close();
};

QT_END_NAMESPACE

#endif
