// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qaudiosystem_p.h"

#include <QtCore/qdebug.h>
#include <QtMultimedia/qaudiosink.h>
#include <QtMultimedia/qaudiosource.h>
#include <QtMultimedia/private/qplatformaudiodevices_p.h>

QT_BEGIN_NAMESPACE

QPlatformAudioEndpointBase::QPlatformAudioEndpointBase(QAudioDevice device,
                                                       const QAudioFormat &format, QObject *parent)
    : QObject{ parent }, m_audioDevice{ std::move(device) }, m_format{ format }
{
    Q_ASSERT(parent && "QPlatformAudioEndpointBase requires the QAudioSink/QAudioSource as parent");
}

void QPlatformAudioEndpointBase::setError(QAudio::Error err)
{
    if (err == m_error)
        return;
    m_error = err;
}

bool QPlatformAudioEndpointBase::isFormatSupported(const QAudioFormat &format) const
{
    return m_audioDevice.isFormatSupported(format);
}

void QPlatformAudioEndpointBase::updateStreamState(QAudio::State state)
{
    if (m_streamState == state)
        return;

    m_streamState = state;
    inferState();
}

void QPlatformAudioEndpointBase::updateStreamIdle(bool idle, EmitStateSignal emitStateSignal)
{
    if (idle == m_streamIsIdle)
        return;
    m_streamIsIdle = idle;

    if (emitStateSignal == EmitStateSignal::True)
        inferState();
}

void QPlatformAudioEndpointBase::inferState()
{
    // The "state" is derived from two sources:
    // * the m_streamState, as changed by start/stop/suspend/resume
    // * the "idle" state of the stream, as detected by the ringbuffer level
    //
    // we combine these two sources to infer a user-visible "state"
    using State = QtAudio::State;

    State oldState = m_inferredState;

    switch (m_streamState) {
    case State::StoppedState:
        m_inferredState = State::StoppedState;
        break;
    case State::SuspendedState:
        m_inferredState = State::SuspendedState;
        break;
    case State::ActiveState:
        m_inferredState = m_streamIsIdle ? State::IdleState : State::ActiveState;
        break;

    case State::IdleState:
        qCritical() << "Users should not be able to set the state to Idle!";
        Q_UNREACHABLE_RETURN();
    }

    if (oldState != m_inferredState)
        emit stateChanged(m_inferredState);
}

QPlatformAudioSink::QPlatformAudioSink(QAudioDevice device, const QAudioFormat &format,
                                       QObject *parent)
    : QPlatformAudioEndpointBase(std::move(device), format, parent)
{
}

QPlatformAudioSink *QPlatformAudioSink::get(const QAudioSink &sink)
{
    return sink.d;
}

QPlatformAudioSource::QPlatformAudioSource(QAudioDevice device, const QAudioFormat &format,
                                           QObject *parent)
    : QPlatformAudioEndpointBase(std::move(device), format, parent)
{
}

QPlatformAudioSource *QPlatformAudioSource::get(const QAudioSource &source)
{
    return source.d;
}

QT_END_NAMESPACE

#include "moc_qaudiosystem_p.cpp"
