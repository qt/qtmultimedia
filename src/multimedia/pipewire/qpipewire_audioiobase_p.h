// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPIPEWIRE_AUDIOIOBASE_P_H
#define QPIPEWIRE_AUDIOIOBASE_P_H

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


#include <QtMultimedia/private/qaudiosystem_p.h>

#include <QtCore/qdebug.h>
#include <QtCore/qloggingcategory.h>

QT_BEGIN_NAMESPACE

namespace QtPipeWire {

Q_DECLARE_LOGGING_CATEGORY(lcPipewireAudioIO);

class QPipewireAudioDevicePrivate;

// clang-format off
#ifdef __cpp_concepts

template <typename T>
concept QPlatformAudioIOBase = requires(T t, QIODevice *device, qsizetype size, qreal volume, const QAudioFormat &format) {
    { t.start(device) } -> std::same_as<void>;
    { t.start() } -> std::same_as<QIODevice*>;
    { t.stop() } -> std::same_as<void>;
    { t.reset() } -> std::same_as<void>;
    { t.suspend() } -> std::same_as<void>;
    { t.resume() } -> std::same_as<void>;

    { t.setBufferSize(size) } -> std::same_as<void>;
    { t.bufferSize() } -> std::same_as<qsizetype>;
    { t.processedUSecs() } -> std::same_as<qint64>;
    { t.error() } -> std::same_as<QAudio::Error>;
    { t.state() } -> std::same_as<QAudio::State>;
    { t.format() } -> std::same_as<QAudioFormat>;
    { t.setVolume(volume) } -> std::same_as<void>;
    { t.volume() } -> std::same_as<qreal>;
};

static_assert(QPlatformAudioIOBase<QPlatformAudioSource>);
static_assert(QPlatformAudioIOBase<QPlatformAudioSink>);

// CAVEAT: opaque, so we cannot write a concept for it
// template <typename T>
// concept QPipewireAudioStream = requires(T t, qreal volume) {
//     { t.setVolume(volume) } -> std::same_as<void>;
// };

#define DECLARE_TEMPLATE_ARGS template <QPlatformAudioIOBase BaseClass, typename StreamType>
#else
#define DECLARE_TEMPLATE_ARGS template <typename BaseClass, typename StreamType>

#endif
// clang-format on

DECLARE_TEMPLATE_ARGS
class QPipewireAudioIOBase : public BaseClass
{
    using SampleFormat = QAudioFormat::SampleFormat;

protected:
    explicit QPipewireAudioIOBase(const QAudioDevice &, const QAudioFormat &format, QObject *parent);

    // device
    QAudioDevice m_audioDevice;
    std::optional<qsizetype> m_bufferSize;
    std::optional<qsizetype> m_hardwareBufferSize;
    void setBufferSize(qsizetype value) override;
    qsizetype bufferSize() const override;
    const QPipewireAudioDevicePrivate *privateDevice() const;

    // format
    QAudioFormat format() const override;
    const QAudioFormat m_format;

    // volume
    qreal m_volume = 1.0f;
    void setVolume(qreal volume) override;
    qreal volume() const override;

    // streams
    std::shared_ptr<StreamType> m_stream;
    qint64 processedUSecs() const override;

    // error
    QAudio::Error error() const override;
    void updateError(QAudio::Error err);
    QAudio::Error m_error = QAudio::Error::NoError;

    // "idle" detection
    QAudio::State state() const override;

    void setUserOwnedState(QtAudio::State state);
    void streamIdle(bool isIdle);
    void updateState();

    QtAudio::State m_userOwnedState = QtAudio::StoppedState;
    bool m_streamIsIdle = false;
    QtAudio::State m_inferredState = QtAudio::StoppedState;
};

DECLARE_TEMPLATE_ARGS
inline QPipewireAudioIOBase<BaseClass, StreamType>::
QPipewireAudioIOBase(const QAudioDevice &device, const QAudioFormat &format, QObject *parent):
    BaseClass{
        parent,
    },
    m_audioDevice{
        device,
    },
    m_format {
        format,
    }
{
}

DECLARE_TEMPLATE_ARGS
inline QAudioFormat QPipewireAudioIOBase<BaseClass, StreamType>::format() const
{
    return m_format;
}

DECLARE_TEMPLATE_ARGS
inline void QPipewireAudioIOBase<BaseClass, StreamType>::setBufferSize(qsizetype value)
{
    if (value <= 0)
        m_bufferSize = {};
    else
        m_bufferSize = value;
}

DECLARE_TEMPLATE_ARGS
inline qsizetype QPipewireAudioIOBase<BaseClass, StreamType>::bufferSize() const
{
    return m_bufferSize.value_or(-1);
}

DECLARE_TEMPLATE_ARGS
inline const QPipewireAudioDevicePrivate *
QPipewireAudioIOBase<BaseClass, StreamType>::privateDevice() const
{
    return reinterpret_cast<const QPipewireAudioDevicePrivate *>(m_audioDevice.handle());
}

DECLARE_TEMPLATE_ARGS
inline void QPipewireAudioIOBase<BaseClass, StreamType>::setVolume(qreal volume)
{
    m_volume = volume;

    if (m_stream)
        m_stream->setVolume(volume);
}

DECLARE_TEMPLATE_ARGS
inline qreal QPipewireAudioIOBase<BaseClass, StreamType>::volume() const
{
    return m_volume;
}

DECLARE_TEMPLATE_ARGS
inline qint64 QPipewireAudioIOBase<BaseClass, StreamType>::processedUSecs() const
{
    if (m_stream)
        return m_stream->processedDuration().count();
    return 0;
}

DECLARE_TEMPLATE_ARGS
inline QAudio::Error QPipewireAudioIOBase<BaseClass, StreamType>::error() const
{
    return m_error;
}

DECLARE_TEMPLATE_ARGS
inline void QPipewireAudioIOBase<BaseClass, StreamType>::updateError(QAudio::Error err)
{
    if (err == m_error)
        return;

    m_error = err;
    emit BaseClass::errorChanged(err);
}

DECLARE_TEMPLATE_ARGS
inline QAudio::State QPipewireAudioIOBase<BaseClass, StreamType>::state() const
{
    return m_inferredState;
}

DECLARE_TEMPLATE_ARGS
inline void QPipewireAudioIOBase<BaseClass, StreamType>::setUserOwnedState(QAudio::State state)
{
    m_userOwnedState = state;
    updateState();
}

DECLARE_TEMPLATE_ARGS
inline void QPipewireAudioIOBase<BaseClass, StreamType>::streamIdle(bool isIdle)
{
    m_streamIsIdle = isIdle;
    updateState();
}

DECLARE_TEMPLATE_ARGS
inline void QPipewireAudioIOBase<BaseClass, StreamType>::updateState()
{
    // The "state" is derived from two sources:
    // * the m_userOwnedState, as changed by start/stop/suspend/resume
    // * the "idle" state of the stream, as detected by the ringbuffer level
    //
    // we combine these two sources to infer a user-visible "state"
    using State = QtAudio::State;

    State oldState = m_inferredState;

    switch (m_userOwnedState) {
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
        qFatal() << "Users should not be able to set the state to Idle!";
    }

    if (oldState != m_inferredState) {
        qCDebug(lcPipewireAudioIO)
                << "QPipewireAudioIOBase updating state:" << m_inferredState << "user state"
                << m_userOwnedState << (m_streamIsIdle ? "(idle)" : "");
        emit BaseClass::stateChanged(m_inferredState);
    }
}

} // namespace QtPipeWire

QT_END_NAMESPACE

#undef DECLARE_TEMPLATE_ARGS

#endif
