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
#include <QtMultimedia/qaudioformat.h>

#include <QtCore/qelapsedtimer.h>
#include <QtCore/private/qglobal_p.h>

QT_BEGIN_NAMESPACE

class QIODevice;

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
    explicit QPlatformAudioEndpointBase(QObject *parent = nullptr);

    // LATER: can we devirtualize these functions
    virtual QAudio::Error error() const { return m_error; }
    virtual QAudio::State state() const { return m_inferredState; }

protected:
    enum class EmitStateSignal : uint8_t
    {
        True,
        False,
    };

    void setError(QAudio::Error);
    void updateStreamState(QAudio::State);
    void updateStreamIdle(bool idle, EmitStateSignal = EmitStateSignal::True);

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
    explicit QPlatformAudioSink(QObject *parent);
    virtual void start(QIODevice *device) = 0;
    virtual QIODevice* start() = 0;
    virtual void stop() = 0;
    virtual void reset() = 0;
    virtual void suspend() = 0;
    virtual void resume() = 0;
    virtual qsizetype bytesFree() const = 0;
    virtual void setBufferSize(qsizetype value) = 0;
    virtual qsizetype bufferSize() const = 0;
    virtual qint64 processedUSecs() const = 0;
    virtual QAudioFormat format() const = 0;
    virtual void setVolume(qreal) {}
    virtual qreal volume() const;

    QElapsedTimer elapsedTime;
};

class Q_MULTIMEDIA_EXPORT QPlatformAudioSource : public QPlatformAudioEndpointBase
{
public:
    explicit QPlatformAudioSource(QObject *parent);
    virtual void start(QIODevice *device) = 0;
    virtual QIODevice* start() = 0;
    virtual void stop() = 0;
    virtual void reset() = 0;
    virtual void suspend()  = 0;
    virtual void resume() = 0;
    virtual qsizetype bytesReady() const = 0;
    virtual void setBufferSize(qsizetype value) = 0;
    virtual qsizetype bufferSize() const = 0;
    virtual qint64 processedUSecs() const = 0;
    virtual QAudioFormat format() const = 0;
    virtual void setVolume(qreal) = 0;
    virtual qreal volume() const = 0;

    QElapsedTimer elapsedTime;
};

QT_END_NAMESPACE

#endif // QAUDIOSYSTEM_H
