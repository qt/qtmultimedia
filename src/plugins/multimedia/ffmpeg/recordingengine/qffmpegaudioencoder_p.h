// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QFFMPEGAUDIOENCODER_P_H
#define QFFMPEGAUDIOENCODER_P_H

#include "qffmpeg_p.h"
#include "qffmpegencoderthread_p.h"
#include "private/qplatformmediarecorder_p.h"
#include <qaudiobuffer.h>
#include <queue>
#include <chrono>

QT_BEGIN_NAMESPACE

class QMediaEncoderSettings;

namespace QFFmpeg {

class AudioEncoder : public EncoderThread
{
public:
    AudioEncoder(RecordingEngine &recordingEngine, const QAudioFormat &sourceFormat,
                 const QMediaEncoderSettings &settings);

    void addBuffer(const QAudioBuffer &buffer);

protected:
    bool checkIfCanPushFrame() const override;

private:
    void open();

    QAudioBuffer takeBuffer();
    void retrievePackets();
    void updateResampler();

    void init() override;
    void cleanup() override;
    bool hasData() const override;
    void processOne() override;

    void handleAudioData(const uchar *data, int &samplesOffset, int samplesCount);

    void ensurePendingFrame(int availableSamplesCount);

    void writeDataToPendingFrame(const uchar *data, int &samplesOffset, int samplesCount);

    void sendPendingFrameToAVCodec();

private:
    std::queue<QAudioBuffer> m_audioBufferQueue;

    // Arbitrarily chosen to limit audio queue duration
    const std::chrono::microseconds m_maxQueueDuration = std::chrono::seconds(5);

    std::chrono::microseconds m_queueDuration{ 0 };

    AVStream *m_stream = nullptr;
    AVCodecContextUPtr m_codecContext;
    QAudioFormat m_format;

    SwrContextUPtr m_resampler;
    qint64 m_samplesWritten = 0;
    const AVCodec *m_avCodec = nullptr;
    QMediaEncoderSettings m_settings;

    AVFrameUPtr m_avFrame;
    int m_avFrameSamplesOffset = 0;
    std::vector<uint8_t *> m_avFramePlanesData;
};


} // namespace QFFmpeg

QT_END_NAMESPACE

#endif
