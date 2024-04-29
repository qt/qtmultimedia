// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QFFMPEGAUDIOENCODER_P_H
#define QFFMPEGAUDIOENCODER_P_H

#include "qffmpeg_p.h"
#include "qffmpegencoderthread_p.h"
#include "private/qplatformmediarecorder_p.h"
#include <qaudiobuffer.h>
#include <queue>

QT_BEGIN_NAMESPACE

class QMediaEncoderSettings;
class QFFmpegAudioInput;

namespace QFFmpeg {

class AudioEncoder : public EncoderThread
{
public:
    AudioEncoder(RecordingEngine &recordingEngine, QFFmpegAudioInput *input,
                 const QMediaEncoderSettings &settings);

    void open();
    void addBuffer(const QAudioBuffer &buffer);

    QFFmpegAudioInput *audioInput() const { return m_input; }

private:
    QAudioBuffer takeBuffer();
    void retrievePackets();

    void init() override;
    void cleanup() override;
    bool hasData() const override;
    void processOne() override;

private:
    std::queue<QAudioBuffer> m_audioBufferQueue;

    AVStream *m_stream = nullptr;
    AVCodecContextUPtr m_codecContext;
    QFFmpegAudioInput *m_input = nullptr;
    QAudioFormat m_format;

    SwrContextUPtr m_resampler;
    qint64 m_samplesWritten = 0;
    const AVCodec *m_avCodec = nullptr;
    QMediaEncoderSettings m_settings;
};


} // namespace QFFmpeg

QT_END_NAMESPACE

#endif
