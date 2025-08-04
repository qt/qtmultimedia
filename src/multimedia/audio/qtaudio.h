// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only


#ifndef QTAUDIO_H
#define QTAUDIO_H

#if 0
#pragma qt_class(QtAudio)
#endif

#include <QtMultimedia/qaudio.h>
#include <QtCore/qspan.h>

#include <functional>
#include <QtCore/q20type_traits.h>

QT_BEGIN_NAMESPACE

namespace QtAudioPrivate {

////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename SampleType>
using AudioSinkCallbackType = std::function<void(QSpan<SampleType>)>;

template <typename SampleType>
using AudioSourceCallbackType = std::function<void(QSpan<const SampleType>)>;

using AudioSinkCallback =
        std::variant<AudioSinkCallbackType<float>, AudioSinkCallbackType<uint8_t>,
                     AudioSinkCallbackType<int16_t>, AudioSinkCallbackType<int32_t>>;

using AudioSourceCallback =
        std::variant<AudioSourceCallbackType<float>, AudioSourceCallbackType<uint8_t>,
                     AudioSourceCallbackType<int16_t>, AudioSourceCallbackType<int32_t>>;

////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename F, typename Enabler = void>
struct GetSampleType;

template <typename F>
struct GetSampleType<
        F,
        std::enable_if_t<std::disjunction_v<std::is_invocable_r<void, F, QSpan<float>>,
                                            std::is_invocable_r<void, F, QSpan<const float>>>>>
    : q20::type_identity<float>
{
};

template <typename F>
struct GetSampleType<
        F,
        std::enable_if_t<std::disjunction_v<std::is_invocable_r<void, F, QSpan<uint8_t>>,
                                            std::is_invocable_r<void, F, QSpan<const uint8_t>>>>>
    : q20::type_identity<uint8_t>
{
};

template <typename F>
struct GetSampleType<
        F,
        std::enable_if_t<std::disjunction_v<std::is_invocable_r<void, F, QSpan<int16_t>>,
                                            std::is_invocable_r<void, F, QSpan<const int16_t>>>>>
    : q20::type_identity<int16_t>
{
};

template <typename F>
struct GetSampleType<
        F,
        std::enable_if_t<std::disjunction_v<std::is_invocable_r<void, F, QSpan<int32_t>>,
                                            std::is_invocable_r<void, F, QSpan<const int32_t>>>>>
    : q20::type_identity<int32_t>
{
};

////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
constexpr inline bool is_std_function_v = false;
template <typename T>
constexpr inline bool is_std_function_v<std::function<T>> = true;
template <typename T>
constexpr inline bool is_function_pointer_v =
        std::conjunction_v<std::is_pointer<T>, std::is_function<std::remove_pointer_t<T>>>;

template <typename F>
constexpr bool isNonnullFunction(const F &f)
{
    if constexpr (is_std_function_v<F> || is_function_pointer_v<F>)
        return bool(f);
    else
        return true; // assume callable is non-null
}

template <typename F, typename... T>
constexpr inline bool isInvokableWithSpan =
        std::disjunction_v<std::is_invocable_r<void, F, QSpan<T>>...>;

////////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace QtAudioPrivate

namespace QtAudio {

template <typename F>
constexpr inline bool isAudioSourceCallback =
        QtAudioPrivate::isInvokableWithSpan<F, const float, const uint8_t, const int16_t,
                                            const int32_t>;

template <typename F>
constexpr inline bool isAudioSinkCallback =
        QtAudioPrivate::isInvokableWithSpan<F, float, uint8_t, int16_t, int32_t>;

template <typename Callback>
using if_audio_source_callback = std::enable_if_t<isAudioSourceCallback<Callback>, bool>;

template <typename Callback>
using if_audio_sink_callback = std::enable_if_t<isAudioSinkCallback<Callback>, bool>;

} // namespace QtAudio

QT_END_NAMESPACE

#endif // QTAUDIO_H
