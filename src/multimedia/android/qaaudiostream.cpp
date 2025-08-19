// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qaaudiostream_p.h"

#include "qandroidaudioutil_p.h"

#include <chrono>
#include <dlfcn.h>

QT_BEGIN_NAMESPACE

namespace {

using namespace std::chrono_literals;
constexpr std::chrono::nanoseconds stateChangeTimeout = 1s;
constexpr int bufferSizeInBursts = 3; // Triple buffering

static aaudio_format_t aaudioFormat(const QAudioFormat::SampleFormat sampleFormat)
{
    switch (sampleFormat) {
    case QAudioFormat::Int16:
    case QAudioFormat::UInt8: // NOTE: Requires conversion
        return AAUDIO_FORMAT_PCM_I16;
    case QAudioFormat::Int32:
        return AAUDIO_FORMAT_PCM_I32;
    case QAudioFormat::Float:
        return AAUDIO_FORMAT_PCM_FLOAT;
    case QAudioFormat::Unknown:
    case QAudioFormat::NSampleFormats:
        qWarning() << "Sample format not supported by AAudio, needs converting";
        return AAUDIO_FORMAT_INVALID;
    }
}

void setMMapPolicy(int policy)
{
    if (QNativeInterface::QAndroidApplication::sdkVersion() < 36)
        return;

    auto handle = dlopen("libaaudio.so", RTLD_NOW);
    Q_ASSERT(handle);
    auto guard = qScopeGuard([handle] {
        dlclose(handle);
    });

    auto getPolicy = (int (*)(void))dlsym(handle, "AAudio_getMMapPolicy");
    Q_ASSERT(getPolicy);
    auto currentPolicy = getPolicy();
    if (currentPolicy == policy)
        return; // No need to set

    auto setPolicy = (aaudio_result_t (*)(int))dlsym(handle, "AAudio_setMMapPolicy");
    Q_ASSERT(setPolicy);
    setPolicy(policy);
};

} // namespace

namespace QtAAudio {

Q_LOGGING_CATEGORY(qLcAAudioStream, "qt.multimedia.android.aaudiostream") // FIXME: Deprecated

StreamBuilder::StreamBuilder(QAudioFormat format)
    : format{ format }
{
    AAudio_createStreamBuilder(&m_builder);
    Q_ASSERT(m_builder);
}

StreamBuilder::~StreamBuilder()
{
    AAudioStreamBuilder_delete(m_builder);
}

void StreamBuilder::setupBuilder()
{
    // Set device
    AAudioStreamBuilder_setDeviceId(m_builder, deviceId);

    // Set buffer capacity
    AAudioStreamBuilder_setBufferCapacityInFrames(m_builder, bufferCapacity);
    // TODO: Tuning buffer size at run-time?

    // Set parameters from audio format
    AAudioStreamBuilder_setSampleRate(m_builder, format.sampleRate());
    AAudioStreamBuilder_setChannelCount(m_builder, format.channelCount());
    AAudioStreamBuilder_setFormat(m_builder, aaudioFormat(format.sampleFormat()));

    // Set callbacks
    AAudioStreamBuilder_setDataCallback(m_builder, callback, userData);
    AAudioStreamBuilder_setErrorCallback(m_builder, errorCallback, userData);

    AAudioStreamBuilder_setPerformanceMode(m_builder, AAUDIO_PERFORMANCE_MODE_LOW_LATENCY);
    setMMapPolicy(2); // Also set MMap policy to AUTO

    // Set other parameters if not default
    StreamParameterSet defaultParams;
    if (params.sharingMode != defaultParams.sharingMode)
        AAudioStreamBuilder_setSharingMode(m_builder, params.sharingMode);
    // Set performance mode to low latency if mmap policy cannot be set
    if (params.direction == AAUDIO_DIRECTION_OUTPUT) {
        if (params.outputUsage != defaultParams.outputUsage)
            AAudioStreamBuilder_setUsage(m_builder, params.outputUsage);
        if (params.outputContentType != defaultParams.outputContentType)
            AAudioStreamBuilder_setContentType(m_builder, params.outputContentType);
    } else {
        AAudioStreamBuilder_setDirection(m_builder, params.direction);
        if (params.inputPreset != defaultParams.inputPreset)
            AAudioStreamBuilder_setInputPreset(m_builder, params.inputPreset);
    }
}

Stream::Stream(const StreamBuilder &builder)
{
    // Request stream open
    auto result = AAudioStreamBuilder_openStream(builder.m_builder, &m_stream);
    if (result != AAUDIO_OK) {
        qCWarning(qLcAAudioStream)
                << "Opening stream failed:" << AAudio_convertResultToText(result);
        if (isOpen())
            close();
        return;
    }

    qCDebug(qLcAAudioStream) << "Stream opened:" << m_stream << "for device"
                             << AAudioStream_getDeviceId(m_stream);

    // Verify parameters
    if (builder.deviceId != AAUDIO_UNSPECIFIED
        && AAudioStream_getDeviceId(m_stream) != builder.deviceId) {
        qCWarning(qLcAAudioStream) << "Stream device not correct";
        if (isOpen())
            close();
        return;
    }

    Q_ASSERT(AAudioStream_getSampleRate(m_stream) == builder.format.sampleRate()
             && AAudioStream_getChannelCount(m_stream) == builder.format.channelCount()
             && AAudioStream_getFormat(m_stream) == aaudioFormat(builder.format.sampleFormat()));

    StreamParameterSet defaultParams;
    if ((builder.params.sharingMode == defaultParams.sharingMode
         || AAudioStream_getSharingMode(m_stream) == builder.params.sharingMode)
        && (builder.params.direction == defaultParams.direction
            || AAudioStream_getDirection(m_stream) == builder.params.direction)
        && (builder.params.outputUsage == defaultParams.outputUsage
            || AAudioStream_getUsage(m_stream) == builder.params.outputUsage)
        && (builder.params.outputContentType == defaultParams.outputContentType
            || AAudioStream_getContentType(m_stream) == builder.params.outputContentType))
        m_areStreamParametersRespected = true;

    if (AAudioStream_getPerformanceMode(m_stream) != AAUDIO_PERFORMANCE_MODE_LOW_LATENCY)
        qCWarning(qLcAAudioStream) << "Low latency performance mode not set";

    // Set buffer size
    auto burstSize = AAudioStream_getFramesPerBurst(m_stream);
    AAudioStream_setBufferSizeInFrames(m_stream, burstSize * bufferSizeInBursts);
}

Stream::~Stream()
{
    if (isOpen())
        close();
}

bool Stream::start()
{
    auto result = requestWithExpectedState(AAudioStream_requestStart, AAUDIO_STREAM_STATE_STARTED);
    return result == AAUDIO_OK;
}

void Stream::stop()
{
    // Plays pending data and stops
    requestWithExpectedState(AAudioStream_requestStop, AAUDIO_STREAM_STATE_STOPPED);
}

void Stream::pause()
{
    // Will freeze data flow
    requestWithExpectedState(AAudioStream_requestPause, AAUDIO_STREAM_STATE_PAUSED);
}

void Stream::flush()
{
    // Discards pending data
    requestWithExpectedState(AAudioStream_requestFlush, AAUDIO_STREAM_STATE_FLUSHED);
}

bool Stream::isOpen() const
{
    return static_cast<bool>(m_stream);
}

bool Stream::areStreamParametersRespected() const
{
    return m_areStreamParametersRespected;
}

void Stream::close()
{
    Q_ASSERT(isOpen());

    AAudioStream_close(m_stream);
    m_stream = nullptr;
}

aaudio_result_t Stream::waitForTargetState(aaudio_stream_state_t targetState)
{
    Q_ASSERT(isOpen());

    aaudio_result_t result = AAUDIO_OK;
    aaudio_stream_state_t currentState = AAudioStream_getState(m_stream);
    aaudio_stream_state_t inputState = currentState;

    while (result == AAUDIO_OK && currentState != targetState) {
        qCDebug(qLcAAudioStream) << "Waiting for state change:"
                                 << AAudio_convertStreamStateToText(currentState) << " -> "
                                 << AAudio_convertStreamStateToText(targetState);
        result = AAudioStream_waitForStateChange(m_stream, inputState, &currentState,
                                                 stateChangeTimeout.count());
        inputState = currentState;
    }

    return result;
}

template <typename Functor>
aaudio_result_t Stream::requestWithExpectedState(Functor &&request, aaudio_stream_state_t expected)
{
    auto result = request(m_stream);
    if (result != AAUDIO_OK) {
        qCWarning(qLcAAudioStream) << "Request failed:" << AAudio_convertResultToText(result);
        return result;
    }

    return waitForTargetState(expected);
}

} // namespace QtAAudio

QT_END_NAMESPACE
