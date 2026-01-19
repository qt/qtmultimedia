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
#include <QtMultimedia/private/qaudiohelpers_p.h>
#include <QtMultimedia/private/qaudio_rtsan_support_p.h>

#include <QtCore/qelapsedtimer.h>
#include <QtCore/qspan.h>
#include <QtCore/private/qglobal_p.h>

#include <array>
#include <variant>

#include <stdlib.h>
#if __has_include(<alloca.h>)
#  include <alloca.h>
#endif
#if __has_include(<malloc.h>)
#  include <malloc.h>
#endif

#if defined(Q_CC_MSVC) && !defined(alloca)
#  define alloca _alloca
#endif

QT_BEGIN_NAMESPACE

class QIODevice;
class QAudioSink;
class QAudioSource;

namespace QtMultimediaPrivate {

using QtAudioPrivate::AudioSinkCallbackType;
using QtAudioPrivate::AudioSourceCallbackType;

#if __cpp_lib_move_only_function

template <typename SampleType>
using AudioSinkMoveOnlyCallbackType = std::move_only_function<void(QSpan<SampleType>)>;

template <typename SampleType>
using AudioSourceMoveOnlyCallbackType = std::move_only_function<void(QSpan<const SampleType>)>;

using AudioSinkMoveOnlyCallback =
        std::variant<AudioSinkMoveOnlyCallbackType<float>, AudioSinkMoveOnlyCallbackType<uint8_t>,
                     AudioSinkMoveOnlyCallbackType<int16_t>,
                     AudioSinkMoveOnlyCallbackType<int32_t>>;

using AudioSourceMoveOnlyCallback = std::variant<
        AudioSourceMoveOnlyCallbackType<float>, AudioSourceMoveOnlyCallbackType<uint8_t>,
        AudioSourceMoveOnlyCallbackType<int16_t>, AudioSourceMoveOnlyCallbackType<int32_t>>;

template <typename V1, typename V2>
struct variant_cat;

template <typename... Ts1, typename... Ts2>
struct variant_cat<std::variant<Ts1...>, std::variant<Ts2...>>
{
    using type = std::variant<Ts1..., Ts2...>;
};

template <typename V1, typename V2>
using variant_cat_t = typename variant_cat<V1, V2>::type;

#endif

///////////////////////////////////////////////////////////////////////////////////////////////////

enum class AudioEndpointRole : uint8_t
{
    MediaPlayback,
    SoundEffect,
    Accessibility,
    Other,
};

///////////////////////////////////////////////////////////////////////////////////////////////////

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

#if __cpp_lib_move_only_function

template <typename T>
struct GetSampleTypeImpl<AudioSinkMoveOnlyCallbackType<T>> : GetSampleTypeImpl<T>
{
};

template <typename T>
struct GetSampleTypeImpl<AudioSourceMoveOnlyCallbackType<T>> : GetSampleTypeImpl<T>
{
};

#endif

template <typename SampleTypeOrCallbackType>
using GetSampleType = typename GetSampleTypeImpl<SampleTypeOrCallbackType>::type;

template <typename SampleTypeOrCallbackType>
static constexpr QAudioFormat::SampleFormat getSampleFormat()
{
    return GetSampleTypeImpl<SampleTypeOrCallbackType>::sample_format;
}

#if __cpp_lib_move_only_function
using AudioSinkCallback =
        variant_cat_t<QtAudioPrivate::AudioSinkCallback, AudioSinkMoveOnlyCallback>;
using AudioSourceCallback =
        variant_cat_t<QtAudioPrivate::AudioSourceCallback, AudioSourceMoveOnlyCallback>;
#else
using AudioSinkCallback = QtAudioPrivate::AudioSinkCallback;
using AudioSourceCallback = QtAudioPrivate::AudioSourceCallback;
#endif

template <typename Callback>
inline AudioSinkCallback asAudioSinkCallback(Callback &&cb)
{
    if constexpr (std::is_same_v<Callback, AudioSinkCallback>) {
        return std::forward<Callback>(cb);
    } else {
        return std::visit([](auto &&arg) -> AudioSinkCallback {
            return arg;
        }, std::forward<Callback>(cb));
    }
}
template <typename Callback>
inline AudioSourceCallback asAudioSourceCallback(Callback &&cb)
{
    if constexpr (std::is_same_v<Callback, AudioSourceCallback>) {
        return std::forward<Callback>(cb);
    } else {
        return std::visit([](auto &&arg) -> AudioSourceCallback {
            return arg;
        }, std::forward<Callback>(cb));
    }
}

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

// if the requested buffer is reasonably small (64kb, big enougth for 16 channels, 1024 frames,
// float32) we can use a stack-allocated temporary buffer.
// otherwise we allocate a heap buffer.
template <size_t limit = 1024 * 64, typename Functor>
inline auto withTemporaryBuffer(size_t bufferSize, Functor &&f) noexcept QT_MM_NONBLOCKING
{
    if (bufferSize <= limit) Q_LIKELY_BRANCH {
#ifdef alloca
        std::byte *stackBuffer = reinterpret_cast<std::byte *>(alloca(bufferSize));
        auto stackBufferSpan = QSpan<std::byte>{
            stackBuffer,
            qsizetype(bufferSize),
        };
#else
        std::array<std::byte, limit> stackArray;
        auto stackBufferSpan = QSpan<std::byte>{
            stackArray,
            qsizetype(bufferSize),
        };
#endif
        return f(stackBufferSpan);
    } else {
        QtPrivate::ScopedRTSanDisabler allowAllocations;
        auto heapBuffer = q20::make_unique_for_overwrite<std::byte[]>(bufferSize);
        auto heapBufferSpan = QSpan<std::byte>{
            heapBuffer.get(),
            qsizetype(bufferSize),
        };
        return f(heapBufferSpan);
    }
}

template <bool IsSink>
inline void
runAudioCallback(std::conditional_t<IsSink, AudioSinkCallback, AudioSourceCallback> &audioCallback,
                 QSpan<std::conditional_t<IsSink, std::byte, const std::byte>> hostBuffer,
                 const QAudioFormat &format)
{
    Q_ASSERT(!hostBuffer.empty());

    bool callbackIsValid = validateAudioCallback(audioCallback, format);
    Q_PRESUME(callbackIsValid);

    int numberOfSamples = format.framesForBytes(hostBuffer.size()) * format.channelCount();

    std::visit([&](auto &callback) {
        using FunctorType = std::decay_t<decltype(callback)>;
        Q_ASSERT(getSampleFormat<FunctorType>() == format.sampleFormat());

        using SampleType = GetSampleType<FunctorType>;

        bool audioCallbackIsValid = bool(callback);
        Q_PRESUME(audioCallbackIsValid);
        using HostBufferType = std::conditional_t<IsSink, SampleType, const SampleType>;

        auto buffer = QSpan<HostBufferType>{
            reinterpret_cast<HostBufferType *>(hostBuffer.data()),
            numberOfSamples,
        };
        callback(buffer);
    }, audioCallback);
}

inline void runAudioCallback(AudioSinkCallback &audioCallback, QSpan<std::byte> hostBuffer,
                             const QAudioFormat &format, float volume)
{
    runAudioCallback<true>(audioCallback, hostBuffer, format);
    QAudioHelperInternal::applyVolume(volume, format, hostBuffer, hostBuffer);
}

inline void runAudioCallback(AudioSinkCallback &audioCallback, QSpan<std::byte> hostBuffer,
                             const QAudioFormat &applicationFormat, float volume,
                             const QAudioFormat &hostFormat)
{
    using namespace QAudioHelperInternal;
    const int32_t numberOfFrames = hostFormat.framesForBytes(hostBuffer.size());
    const int32_t applicationBufferSize = applicationFormat.bytesForFrames(numberOfFrames);

    withTemporaryBuffer(applicationBufferSize, [&](QSpan<std::byte> tempBuffer) {
        runAudioCallback(audioCallback, tempBuffer, applicationFormat, volume);
        convertSampleFormat(tempBuffer, toNativeSampleFormat(applicationFormat.sampleFormat()),
                            hostBuffer, toNativeSampleFormat(hostFormat.sampleFormat()));
    });
}

// NB: we we provide two overloads for running audio callbacks based on the host buffer:
// * if the host buffer is immutable, we need to apply the volume on a temporary buffer
// * if the host buffer is mutable, we can apply the volume in-place (currently unused)
inline void runAudioCallback(AudioSourceCallback &audioCallback, QSpan<const std::byte> hostBuffer,
                             const QAudioFormat &format, float volume)
{
    if (volume == 1.0f) {
        runAudioCallback<false>(audioCallback, hostBuffer, format);
    } else {
        withTemporaryBuffer(hostBuffer.size(), [&](QSpan<std::byte> tempBuffer) {
            QAudioHelperInternal::applyVolume(volume, format, hostBuffer, tempBuffer);
            runAudioCallback<false>(audioCallback, tempBuffer, format);
        });
    }
}

inline void runAudioCallback(AudioSourceCallback &audioCallback, QSpan<std::byte> hostBuffer,
                             const QAudioFormat &format, float volume)
{
    QAudioHelperInternal::applyVolume(volume, format, hostBuffer, hostBuffer);
    runAudioCallback<false>(audioCallback, hostBuffer, format);
}

template <typename HostBufferType>
inline void runAudioCallback(AudioSourceCallback &audioCallback, QSpan<HostBufferType> hostBuffer,
                             const QAudioFormat &applicationFormat, float volume,
                             const QAudioFormat &hostFormat)
{
    static_assert(std::is_same_v<HostBufferType, std::byte>
                  || std::is_same_v<HostBufferType, const std::byte>);

    using namespace QAudioHelperInternal;

    const int32_t numberOfFrames = hostFormat.framesForBytes(hostBuffer.size());
    const int32_t applicationBufferSize = applicationFormat.bytesForFrames(numberOfFrames);

    withTemporaryBuffer(applicationBufferSize, [&](QSpan<std::byte> tempBuffer) {
        convertSampleFormat(hostBuffer, toNativeSampleFormat(hostFormat.sampleFormat()), tempBuffer,
                            toNativeSampleFormat(applicationFormat.sampleFormat()));
        runAudioCallback(audioCallback, tempBuffer, applicationFormat, volume);
    });
}

///////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace QtMultimediaPrivate

class Q_MULTIMEDIA_EXPORT QPlatformAudioEndpointBase : public QObject
{
    Q_OBJECT

public:
    explicit QPlatformAudioEndpointBase(QAudioDevice, const QAudioFormat &, QObject *parent);
    ~QPlatformAudioEndpointBase() override;

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
    Q_OBJECT

public:
    explicit QPlatformAudioSink(QAudioDevice, const QAudioFormat &, QObject *parent);
    ~QPlatformAudioSink() override;

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
    Q_OBJECT

public:
    explicit QPlatformAudioSource(QAudioDevice, const QAudioFormat &, QObject *parent);
    ~QPlatformAudioSource() override;

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
