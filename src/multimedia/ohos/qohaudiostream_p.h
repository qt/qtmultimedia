// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHAUDIOSTREAM_P_H
#define QOHAUDIOSTREAM_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtMultimedia/qaudioformat.h>

#include <QtCore/qloggingcategory.h>

#include <ohaudio/native_audiocapturer.h>
#include <ohaudio/native_audiorenderer.h>
#include <ohaudio/native_audiostream_base.h>
#include <ohaudio/native_audiostreambuilder.h>

QT_BEGIN_NAMESPACE

namespace QtOHAudio {

Q_DECLARE_LOGGING_CATEGORY(qLcOHAudioStream)

struct StreamBuilder;

struct Stream
{
    explicit Stream(StreamBuilder &builder);
    ~Stream();

    Q_DISABLE_COPY_MOVE(Stream)

    bool start();
    void stop();
    void pause();
    void flush();

    bool isOpen() const;
    bool areStreamParametersRespected() const;

    OH_AudioRenderer *renderer() const noexcept { return m_renderer; }
    OH_AudioCapturer *capturer() const noexcept { return m_capturer; }

private:
    void close();

    OH_AudioStream_Type m_streamType{ AUDIOSTREAM_TYPE_RENDERER };
    OH_AudioRenderer *m_renderer{ nullptr };
    OH_AudioCapturer *m_capturer{ nullptr };
    bool m_areStreamParametersRespected{ false };
};

struct StreamParameterSet
{
    OH_AudioStream_Type direction{ AUDIOSTREAM_TYPE_RENDERER };
    OH_AudioStream_Usage outputUsage{ AUDIOSTREAM_USAGE_MUSIC };
    OH_AudioStream_SourceType inputSourceType{ AUDIOSTREAM_SOURCE_TYPE_MIC };
    OH_AudioStream_LatencyMode latencyMode{ AUDIOSTREAM_LATENCY_MODE_NORMAL };
};

struct StreamBuilder
{
    friend Stream;

    explicit StreamBuilder(QAudioFormat format, OH_AudioStream_Type direction);
    ~StreamBuilder();

    Q_DISABLE_COPY_MOVE(StreamBuilder)

    QAudioFormat format;
    OH_AudioRenderer_OnWriteDataCallback writeCallback{ nullptr };
    int32_t (*readCallback)(OH_AudioCapturer *capturer, void *userData, void *buffer,
                            int32_t length){ nullptr };
    void *userData{ nullptr };
    StreamParameterSet params;
    int32_t deviceId{ 0 };

    void setupBuilder();

private:
    OH_AudioStreamBuilder *m_builder{ nullptr };
};

OH_AudioStream_SampleFormat toOHSampleFormat(QAudioFormat::SampleFormat fmt);
QAudioFormat::SampleFormat fromOHSampleFormat(OH_AudioStream_SampleFormat fmt);
QAudioFormat::SampleFormat preferredCompatibleSampleFormat(QAudioFormat::SampleFormat requested);

} // namespace QtOHAudio

QT_END_NAMESPACE

#endif // QOHAUDIOSTREAM_P_H
