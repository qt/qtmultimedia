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
#include <QtMultimedia/private/qmultimedia_assume_p.h>

#include <QtCore/qelapsedtimer.h>
#include <QtCore/qspan.h>
#include <QtCore/private/qglobal_p.h>

#include <functional>
#include <variant>

QT_BEGIN_NAMESPACE

class QIODevice;
class QAudioSink;
class QAudioSource;

namespace QtMultimediaPrivate {

///////////////////////////////////////////////////////////////////////////////////////////////////

enum class AudioEndpointRole : uint8_t
{
    MediaPlayback,
    SoundEffect,
    Accessibility,
    Other,
};

///////////////////////////////////////////////////////////////////////////////////////////////////

template <typename SampleType>
using AudioSinkCallbackType = std::function<void(QSpan<SampleType>)>;

template <typename SampleType>
using AudioSourceCallbackType = std::function<void(QSpan<const SampleType>)>;

template <typename>
struct GetSampleTypeImpl;

template <>
struct GetSampleTypeImpl<float>
{
    using type = float;
    static constexpr QAudioFormat::SampleFormat sample_format = QAudioFormat::SampleFormat::Float;
};

template <>
struct GetSampleTypeImpl<int32_t>
{
    using type = int32_t;
    static constexpr QAudioFormat::SampleFormat sample_format = QAudioFormat::SampleFormat::Int32;
};
template <>
struct GetSampleTypeImpl<int16_t>
{
    using type = int16_t;
    static constexpr QAudioFormat::SampleFormat sample_format = QAudioFormat::SampleFormat::Int16;
};

template <>
struct GetSampleTypeImpl<uint8_t>
{
    using type = uint8_t;
    static constexpr QAudioFormat::SampleFormat sample_format = QAudioFormat::SampleFormat::UInt8;
};

template <typename T>
struct GetSampleTypeImpl<AudioSinkCallbackType<T>> : GetSampleTypeImpl<T>
{
};

template <typename T>
struct GetSampleTypeImpl<AudioSourceCallbackType<T>> : GetSampleTypeImpl<T>
{
};

template <typename SampleTypeOrCallbackType>
using GetSampleType = typename GetSampleTypeImpl<SampleTypeOrCallbackType>::type;

template <typename SampleTypeOrCallbackType>
static constexpr QAudioFormat::SampleFormat getSampleFormat()
{
    return GetSampleTypeImpl<SampleTypeOrCallbackType>::sample_format;
}

using AudioSinkCallback =
        std::variant<AudioSinkCallbackType<float>, AudioSinkCallbackType<uint8_t>,
                     AudioSinkCallbackType<int16_t>, AudioSinkCallbackType<int32_t>>;
using AudioSourceCallback =
        std::variant<AudioSourceCallbackType<float>, AudioSourceCallbackType<uint8_t>,
                     AudioSourceCallbackType<int16_t>, AudioSourceCallbackType<int32_t>>;

template <typename AnyAudioCallback>
constexpr bool validateAudioCallbackImpl(const AnyAudioCallback &audioCallback,
                                         const QAudioFormat &format)
{
    bool isNonNullFunction = std::visit([](const auto &cb) {
        return bool(cb);
    }, audioCallback);

    if (!isNonNullFunction)
        return false;

    bool hasCorrectFormat = std::visit([&](const auto &cb) {
        return getSampleFormat<std::decay_t<decltype(cb)>>() == format.sampleFormat();
    }, audioCallback);

    return hasCorrectFormat;
}

constexpr bool validateAudioCallback(const AudioSinkCallback &audioCallback,
                                     const QAudioFormat &format)
{
    return validateAudioCallbackImpl(audioCallback, format);
}

constexpr bool validateAudioCallback(const AudioSourceCallback &audioCallback,
                                     const QAudioFormat &format)
{
    return validateAudioCallbackImpl(audioCallback, format);
}

template <bool IsSink>
inline void runAudioCallback(
        const std::conditional_t<IsSink, AudioSinkCallback, AudioSourceCallback> &audioCallback,
        QSpan<std::conditional_t<IsSink, std::byte, const std::byte>> hostBuffer,
        const QAudioFormat &format)
{
    Q_ASSERT(!hostBuffer.empty());

    bool callbackIsValid = validateAudioCallback(audioCallback, format);
    QT_MM_ASSUME(callbackIsValid);

    int numberOfSamples = format.framesForBytes(hostBuffer.size()) * format.channelCount();

    std::visit([&](const auto &callback) {
        using FunctorType = std::decay_t<decltype(callback)>;
        Q_ASSERT(getSampleFormat<FunctorType>() == format.sampleFormat());

        using SampleType = GetSampleType<FunctorType>;

        bool audioCallbackIsValid = bool(callback);
        QT_MM_ASSUME(audioCallbackIsValid);
        using HostBufferType = std::conditional_t<IsSink, SampleType, const SampleType>;

        auto buffer = QSpan<HostBufferType>{
            reinterpret_cast<HostBufferType *>(hostBuffer.data()),
            numberOfSamples,
        };
        callback(buffer);
    }, audioCallback);
}

inline void runAudioCallback(const AudioSinkCallback &audioCallback, QSpan<std::byte> hostBuffer,
                             const QAudioFormat &format)
{
    return runAudioCallback<true>(audioCallback, hostBuffer, format);
}

inline void runAudioCallback(const AudioSourceCallback &audioCallback,
                             QSpan<const std::byte> hostBuffer, const QAudioFormat &format)
{
    return runAudioCallback<false>(audioCallback, hostBuffer, format);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace QtMultimediaPrivate

class Q_MULTIMEDIA_EXPORT QPlatformAudioEndpointBase : public QObject
{
    Q_OBJECT

public:
    explicit QPlatformAudioEndpointBase(QAudioDevice, const QAudioFormat &, QObject *parent);

    // LATER: can we devirtualize these functions
    QAudio::Error error() const { return m_error; }
    virtual QAudio::State state() const { return m_inferredState; }
    virtual void setError(QAudio::Error);

    virtual bool isFormatSupported(const QAudioFormat &format) const;
    QAudioFormat format() const { return m_format; }

    virtual void setVolume(float volume) { m_volume = volume; }
    float volume() const { return m_volume; }

Q_SIGNALS:
    void stateChanged(QtAudio::State);

protected:
    enum class EmitStateSignal : uint8_t
    {
        True,
        False,
    };

    void updateStreamState(QAudio::State);
    void updateStreamIdle(bool idle, EmitStateSignal = EmitStateSignal::True);

    const QAudioDevice m_audioDevice;
    const QAudioFormat m_format;

private:
    void inferState();

    QAudio::State m_streamState = QAudio::StoppedState;
    QAudio::State m_inferredState = QAudio::StoppedState;
    QAudio::Error m_error{};
    bool m_streamIsIdle = false;

    float m_volume{ 1.f };
};

class Q_MULTIMEDIA_EXPORT QPlatformAudioSink : public QPlatformAudioEndpointBase
{
public:
    explicit QPlatformAudioSink(QAudioDevice, const QAudioFormat &, QObject *parent);
    virtual void start(QIODevice *device) = 0;
    virtual QIODevice* start() = 0;
    virtual void stop() = 0;
    virtual void reset() = 0;
    virtual void suspend() = 0;
    virtual void resume() = 0;
    virtual qsizetype bytesFree() const = 0;
    virtual void setBufferSize(qsizetype value) = 0;
    virtual qsizetype bufferSize() const = 0;
    virtual void setHardwareBufferFrames(int32_t) { }
    virtual int32_t hardwareBufferFrames() { return -1; }
    virtual qint64 processedUSecs() const = 0;

    using AudioCallback = QtMultimediaPrivate::AudioSinkCallback;

    virtual void start(AudioCallback &&) { }
    virtual bool hasCallbackAPI() { return false; }

    QElapsedTimer elapsedTime;

    static QPlatformAudioSink *get(const QAudioSink &);

    using AudioEndpointRole = QtMultimediaPrivate::AudioEndpointRole;
    virtual void setRole(AudioEndpointRole) { }
};

class Q_MULTIMEDIA_EXPORT QPlatformAudioSource : public QPlatformAudioEndpointBase
{
public:
    explicit QPlatformAudioSource(QAudioDevice, const QAudioFormat &, QObject *parent);
    virtual void start(QIODevice *device) = 0;
    virtual QIODevice* start() = 0;
    virtual void stop() = 0;
    virtual void reset() = 0;
    virtual void suspend()  = 0;
    virtual void resume() = 0;
    virtual qsizetype bytesReady() const = 0;
    virtual void setBufferSize(qsizetype value) = 0;
    virtual void setHardwareBufferFrames(int32_t) { }
    virtual int32_t hardwareBufferFrames() { return -1; }
    virtual qsizetype bufferSize() const = 0;
    virtual qint64 processedUSecs() const = 0;

    using AudioCallback = QtMultimediaPrivate::AudioSourceCallback;

    virtual void start(AudioCallback &&) { }
    virtual bool hasCallbackAPI() { return false; }

    QElapsedTimer elapsedTime;

    static QPlatformAudioSource *get(const QAudioSource &);
};

// forward declarations
namespace QtMultimediaPrivate {
class QPlatformAudioSinkStream;
class QPlatformAudioSourceStream;
} // namespace QtMultimediaPrivate

QT_END_NAMESPACE

#endif // QAUDIOSYSTEM_H
