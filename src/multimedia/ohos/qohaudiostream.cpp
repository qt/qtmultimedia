// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohaudiostream_p.h"

#include <QtCore/qloggingcategory.h>
#include <QtCore/qmutex.h>

QT_BEGIN_NAMESPACE

namespace QtOHAudio {

Q_LOGGING_CATEGORY(qLcOHAudioStream, "qt.multimedia.ohos.audiostream")

namespace {
// OH_AudioStreamBuilder is not thread-safe: concurrent Create/Destroy and
// GenerateRenderer/GenerateCapturer calls from multiple threads crash inside
// the audio service. Serialize all builder lifecycle operations.
Q_GLOBAL_STATIC(QMutex, g_streamBuilderMutex)
}

OH_AudioStream_SampleFormat toOHSampleFormat(QAudioFormat::SampleFormat fmt)
{
    switch (fmt) {
    case QAudioFormat::UInt8:
        return AUDIOSTREAM_SAMPLE_U8;
    case QAudioFormat::Int16:
        return AUDIOSTREAM_SAMPLE_S16LE;
    case QAudioFormat::Int32:
        return AUDIOSTREAM_SAMPLE_S32LE;
    case QAudioFormat::Float:
        return AUDIOSTREAM_SAMPLE_F32LE;
    case QAudioFormat::Unknown:
    case QAudioFormat::NSampleFormats:
        break;
    }
    return AUDIOSTREAM_SAMPLE_S16LE;
}

QAudioFormat::SampleFormat fromOHSampleFormat(OH_AudioStream_SampleFormat fmt)
{
    switch (fmt) {
    case AUDIOSTREAM_SAMPLE_U8:
        return QAudioFormat::UInt8;
    case AUDIOSTREAM_SAMPLE_S16LE:
        return QAudioFormat::Int16;
    case AUDIOSTREAM_SAMPLE_S24LE:
        return QAudioFormat::Int32;
    case AUDIOSTREAM_SAMPLE_S32LE:
        return QAudioFormat::Int32;
    case AUDIOSTREAM_SAMPLE_F32LE:
        return QAudioFormat::Float;
    }
    return QAudioFormat::Unknown;
}

QAudioFormat::SampleFormat preferredCompatibleSampleFormat(QAudioFormat::SampleFormat requested)
{
    switch (requested) {
    case QAudioFormat::UInt8:
    case QAudioFormat::Int16:
    case QAudioFormat::Int32:
    case QAudioFormat::Float:
        return requested;
    case QAudioFormat::Unknown:
    case QAudioFormat::NSampleFormats:
        break;
    }
    return QAudioFormat::Int16;
}

const char *resultToString(OH_AudioStream_Result result)
{
    switch (result) {
    case AUDIOSTREAM_SUCCESS:
        return "AUDIOSTREAM_SUCCESS";
    case AUDIOSTREAM_ERROR_INVALID_PARAM:
        return "AUDIOSTREAM_ERROR_INVALID_PARAM";
    case AUDIOSTREAM_ERROR_ILLEGAL_STATE:
        return "AUDIOSTREAM_ERROR_ILLEGAL_STATE";
    case AUDIOSTREAM_ERROR_SYSTEM:
        return "AUDIOSTREAM_ERROR_SYSTEM";
    case AUDIOSTREAM_ERROR_UNSUPPORTED_FORMAT:
        return "AUDIOSTREAM_ERROR_UNSUPPORTED_FORMAT";
    }
    return "AUDIOSTREAM_UNKNOWN_RESULT";
}

StreamBuilder::StreamBuilder(QAudioFormat fmt, OH_AudioStream_Type direction)
    : format{ std::move(fmt) }
{
    params.direction = direction;
    QMutexLocker lock{ g_streamBuilderMutex() };
    OH_AudioStream_Result result = OH_AudioStreamBuilder_Create(&m_builder, direction);
    if (result != AUDIOSTREAM_SUCCESS)
        qCWarning(qLcOHAudioStream)
                << "Failed to create stream builder:" << resultToString(result);
}

StreamBuilder::~StreamBuilder()
{
    QMutexLocker lock{ g_streamBuilderMutex() };
    if (m_builder)
        OH_AudioStreamBuilder_Destroy(m_builder);
}

void StreamBuilder::setupBuilder()
{
    if (!m_builder)
        return;

    OH_AudioStreamBuilder_SetSamplingRate(m_builder, format.sampleRate());
    OH_AudioStreamBuilder_SetChannelCount(m_builder, format.channelCount());
    OH_AudioStreamBuilder_SetSampleFormat(m_builder, toOHSampleFormat(format.sampleFormat()));
    OH_AudioStreamBuilder_SetEncodingType(m_builder, AUDIOSTREAM_ENCODING_TYPE_RAW);
    OH_AudioStreamBuilder_SetLatencyMode(m_builder, params.latencyMode);

    // TODO: register the OH_AudioRenderer_Callbacks / OH_AudioCapturer_Callbacks
    // error and interrupt-event handlers so the backend can react to focus loss,
    // device changes and stream errors. See:
    // https://developer.huawei.com/consumer/en/doc/harmonyos-references/capi-ohaudio-oh-audiorenderer-callbacks-struct
    if (params.direction == AUDIOSTREAM_TYPE_RENDERER) {
        OH_AudioStreamBuilder_SetRendererInfo(m_builder, params.outputUsage);
        if (writeCallback) {
            OH_AudioStreamBuilder_SetRendererWriteDataCallback(m_builder, writeCallback,
                                                               userData);
        }
    } else {
        OH_AudioStreamBuilder_SetCapturerInfo(m_builder, params.inputSourceType);
        if (readCallback) {
            OH_AudioCapturer_Callbacks callbacks{};
            callbacks.OH_AudioCapturer_OnReadData = readCallback;
            OH_AudioStreamBuilder_SetCapturerCallback(m_builder, callbacks, userData);
        }
    }
}

Stream::Stream(StreamBuilder &builder)
    : m_streamType{ builder.params.direction }
{
    if (!builder.m_builder) {
        qCWarning(qLcOHAudioStream) << "Builder is invalid";
        return;
    }

    OH_AudioStream_Result result = AUDIOSTREAM_SUCCESS;
    {
        QMutexLocker lock{ g_streamBuilderMutex() };
        if (m_streamType == AUDIOSTREAM_TYPE_RENDERER) {
            result = OH_AudioStreamBuilder_GenerateRenderer(builder.m_builder, &m_renderer);
        } else {
            result = OH_AudioStreamBuilder_GenerateCapturer(builder.m_builder, &m_capturer);
        }
    }

    if (result != AUDIOSTREAM_SUCCESS) {
        qCWarning(qLcOHAudioStream)
                << "Failed to generate stream:" << resultToString(result);
        return;
    }

    m_areStreamParametersRespected = true;
}

Stream::~Stream()
{
    if (isOpen())
        close();
}

bool Stream::start()
{
    OH_AudioStream_Result result = AUDIOSTREAM_SUCCESS;
    if (m_streamType == AUDIOSTREAM_TYPE_RENDERER)
        result = OH_AudioRenderer_Start(m_renderer);
    else
        result = OH_AudioCapturer_Start(m_capturer);

    if (result != AUDIOSTREAM_SUCCESS)
        qCWarning(qLcOHAudioStream) << "Failed to start stream:" << resultToString(result);

    return result == AUDIOSTREAM_SUCCESS;
}

void Stream::stop()
{
    if (m_streamType == AUDIOSTREAM_TYPE_RENDERER)
        OH_AudioRenderer_Stop(m_renderer);
    else
        OH_AudioCapturer_Stop(m_capturer);
}

void Stream::pause()
{
    OH_AudioStream_Result result = AUDIOSTREAM_SUCCESS;
    if (m_streamType == AUDIOSTREAM_TYPE_RENDERER)
        result = OH_AudioRenderer_Pause(m_renderer);
    else
        result = OH_AudioCapturer_Pause(m_capturer);
    if (result != AUDIOSTREAM_SUCCESS)
        qCWarning(qLcOHAudioStream)
                << "Failed to pause stream:" << resultToString(result);
}

void Stream::flush()
{
    if (m_streamType == AUDIOSTREAM_TYPE_RENDERER)
        OH_AudioRenderer_Flush(m_renderer);
}

bool Stream::isOpen() const
{
    return m_renderer != nullptr || m_capturer != nullptr;
}

bool Stream::areStreamParametersRespected() const
{
    return m_areStreamParametersRespected;
}

void Stream::close()
{
    if (m_streamType == AUDIOSTREAM_TYPE_RENDERER && m_renderer) {
        OH_AudioRenderer_Release(m_renderer);
        m_renderer = nullptr;
    } else if (m_capturer) {
        OH_AudioCapturer_Release(m_capturer);
        m_capturer = nullptr;
    }
}

} // namespace QtOHAudio

QT_END_NAMESPACE
