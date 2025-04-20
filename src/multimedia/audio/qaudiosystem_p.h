// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QAUDIOSYSTEM_H
#define QAUDIOSYSTEM_H

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

#include <QtMultimedia/qtmultimediaglobal.h>

#include <QtMultimedia/qaudio.h>
#include <QtMultimedia/qaudiodevice.h>
#include <QtMultimedia/qaudioformat.h>

#include <QtCore/qelapsedtimer.h>
#include <QtCore/private/qglobal_p.h>

QT_BEGIN_NAMESPACE

class QIODevice;
class QAudioSink;

namespace QtMultimediaPrivate {

enum class AudioEndpointRole : uint8_t {
    MediaPlayback,
    SoundEffect,
    Other,
};

} // namespace QtMultimediaPrivate

class Q_MULTIMEDIA_EXPORT QAudioStateChangeNotifier : public QObject
{
    Q_OBJECT

public:
    explicit QAudioStateChangeNotifier(QObject *parent = nullptr);

signals:
    void errorChanged(QAudio::Error error);
    void stateChanged(QAudio::State state);
};

class Q_MULTIMEDIA_EXPORT QPlatformAudioEndpointBase : public QAudioStateChangeNotifier
{
public:
    explicit QPlatformAudioEndpointBase(QAudioDevice, QObject *parent = nullptr);

    // LATER: can we devirtualize these functions
    QAudio::Error error() const { return m_error; }
    virtual QAudio::State state() const { return m_inferredState; }
    virtual void setError(QAudio::Error);

    virtual bool isFormatSupported(const QAudioFormat &format) const;

protected:
    enum class EmitStateSignal : uint8_t
    {
        True,
        False,
    };

    void updateStreamState(QAudio::State);
    void updateStreamIdle(bool idle, EmitStateSignal = EmitStateSignal::True);

    const QAudioDevice m_audioDevice;

private:
    void inferState();

    QAudio::State m_streamState = QAudio::StoppedState;
    QAudio::State m_inferredState = QAudio::StoppedState;
    QAudio::Error m_error{};
    bool m_streamIsIdle = false;
};

class Q_MULTIMEDIA_EXPORT QPlatformAudioSink : public QPlatformAudioEndpointBase
{
public:
    explicit QPlatformAudioSink(QAudioDevice, QObject *parent);
    virtual void start(QIODevice *device) = 0;
    virtual QIODevice* start() = 0;
    virtual void stop() = 0;
    virtual void reset() = 0;
    virtual void suspend() = 0;
    virtual void resume() = 0;
    virtual qsizetype bytesFree() const = 0;
    virtual void setBufferSize(qsizetype value) = 0;
    virtual qsizetype bufferSize() const = 0;
    virtual void setHardwareBufferFrames(int32_t) { };
    virtual int32_t hardwareBufferFrames() { return -1; };
    virtual qint64 processedUSecs() const = 0;
    virtual QAudioFormat format() const = 0;
    virtual void setVolume(float) { }
    virtual float volume() const;

    QElapsedTimer elapsedTime;

    static QPlatformAudioSink *get(const QAudioSink &);

    using AudioEndpointRole = QtMultimediaPrivate::AudioEndpointRole;
    virtual void setRole(AudioEndpointRole) { }
};

class Q_MULTIMEDIA_EXPORT QPlatformAudioSource : public QPlatformAudioEndpointBase
{
public:
    explicit QPlatformAudioSource(QAudioDevice, QObject *parent);
    virtual void start(QIODevice *device) = 0;
    virtual QIODevice* start() = 0;
    virtual void stop() = 0;
    virtual void reset() = 0;
    virtual void suspend()  = 0;
    virtual void resume() = 0;
    virtual qsizetype bytesReady() const = 0;
    virtual void setBufferSize(qsizetype value) = 0;
    virtual void setHardwareBufferFrames(int32_t) { };
    virtual int32_t hardwareBufferFrames() { return -1; };
    virtual qsizetype bufferSize() const = 0;
    virtual qint64 processedUSecs() const = 0;
    virtual QAudioFormat format() const = 0;
    virtual void setVolume(float) = 0;
    virtual float volume() const = 0;

    QElapsedTimer elapsedTime;
};

// forward declarations
namespace QtMultimediaPrivate {
class QPlatformAudioSinkStream;
class QPlatformAudioSourceStream;
} // namespace QtMultimediaPrivate

QT_END_NAMESPACE

#endif // QAUDIOSYSTEM_H
