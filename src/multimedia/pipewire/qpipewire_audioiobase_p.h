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

#include <QtMultimedia/private/qaudiodevice_p.h>
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
concept QPlatformAudioIOBase = requires(T t, QIODevice *device, qsizetype size, float volume, int32_t hwBufferFrames, const QAudioFormat &format) {
    { t.start(device) } -> std::same_as<void>;
    { t.start() } -> std::same_as<QIODevice*>;
    { t.stop() } -> std::same_as<void>;
    { t.reset() } -> std::same_as<void>;
    { t.suspend() } -> std::same_as<void>;
    { t.resume() } -> std::same_as<void>;

    { t.setBufferSize(size) } -> std::same_as<void>;
    { t.bufferSize() } -> std::same_as<qsizetype>;
    { t.setHardwareBufferFrames(hwBufferFrames) } -> std::same_as<void>;
    { t.hardwareBufferFrames() } -> std::same_as<int32_t>;
    { t.processedUSecs() } -> std::same_as<qint64>;
    { t.error() } -> std::same_as<QAudio::Error>;
    { t.state() } -> std::same_as<QAudio::State>;
    { t.format() } -> std::same_as<QAudioFormat>;
    { t.setVolume(volume) } -> std::same_as<void>;
    { t.volume() } -> std::same_as<float>;
};

static_assert(QPlatformAudioIOBase<QPlatformAudioSource>);
static_assert(QPlatformAudioIOBase<QPlatformAudioSink>);

// CAVEAT: opaque, so we cannot write a concept for it
// template <typename T>
// concept QPipewireAudioStream = requires(T t, float volume) {
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
    explicit QPipewireAudioIOBase(QAudioDevice, const QAudioFormat &format, QObject *parent);

    // device
    std::optional<qsizetype> m_bufferSize;
    void setBufferSize(qsizetype value) override;
    qsizetype bufferSize() const override;

    // format
    QAudioFormat format() const override;
    const QAudioFormat m_format;

    // volume
    float m_volume = 1.0f;
    void setVolume(float volume) override;
    float volume() const override;

    // hw buffer size
    std::optional<qsizetype> m_hardwareBufferFrames;
    void setHardwareBufferFrames(int32_t arg) override;
    int32_t hardwareBufferFrames() override;

    // streams
    std::shared_ptr<StreamType> m_stream;
    qint64 processedUSecs() const override;
};

DECLARE_TEMPLATE_ARGS
inline QPipewireAudioIOBase<BaseClass, StreamType>::
QPipewireAudioIOBase(QAudioDevice device, const QAudioFormat &format, QObject *parent):
    BaseClass{
        std::move(device),
        parent,
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
inline void QPipewireAudioIOBase<BaseClass, StreamType>::setVolume(float volume)
{
    m_volume = volume;

    if (m_stream)
        m_stream->setVolume(volume);
}

DECLARE_TEMPLATE_ARGS
inline float QPipewireAudioIOBase<BaseClass, StreamType>::volume() const
{
    return m_volume;
}

DECLARE_TEMPLATE_ARGS
inline void QPipewireAudioIOBase<BaseClass, StreamType>::setHardwareBufferFrames(int32_t arg)
{
    if (m_hardwareBufferFrames > 0)
        m_hardwareBufferFrames = arg;
    else
        m_hardwareBufferFrames = std::nullopt;
}

DECLARE_TEMPLATE_ARGS
inline int32_t QPipewireAudioIOBase<BaseClass, StreamType>::hardwareBufferFrames()
{
    return m_hardwareBufferFrames.value_or(-1);
}

DECLARE_TEMPLATE_ARGS
inline qint64 QPipewireAudioIOBase<BaseClass, StreamType>::processedUSecs() const
{
    if (m_stream)
        return m_stream->processedDuration().count();
    return 0;
}

} // namespace QtPipeWire

QT_END_NAMESPACE

#undef DECLARE_TEMPLATE_ARGS

#endif
