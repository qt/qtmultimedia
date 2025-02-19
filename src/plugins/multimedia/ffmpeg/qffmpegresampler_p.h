// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QFFMPEGRESAMPLER_P_H
#define QFFMPEGRESAMPLER_P_H

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

#include "qaudiobuffer.h"
#include <QtFFmpegMediaPluginImpl/private/qffmpeg_p.h>
#include <QtMultimedia/private/qplatformaudioresampler_p.h>

QT_BEGIN_NAMESPACE

namespace QFFmpeg
{
class CodecContext;
} // namespace QFFmpeg

class QFFmpegResampler : public QPlatformAudioResampler
{
public:
    QFFmpegResampler(const QAudioFormat &inputFormat, const QAudioFormat &outputFormat,
                     qint64 startTime = 0);
    QFFmpegResampler(const QFFmpeg::CodecContext *codecContext, const QAudioFormat &outputFormat,
                     qint64 startTime = 0);

    ~QFFmpegResampler() override;

    QAudioBuffer resample(const char* data, size_t size) override;

    QAudioBuffer resample(const AVFrame *frame);

    qint64 samplesProcessed() const { return m_samplesProcessed; }
    void setSampleCompensation(qint32 delta, quint32 distance);
    qint32 activeSampleCompensationDelta() const;

private:
    int adjustMaxOutSamples(int inputSamplesCount);

    QAudioBuffer resample(const uint8_t **inputData, int inputSamplesCount);

private:
    QAudioFormat m_inputFormat;
    QAudioFormat m_outputFormat;
    qint64 m_startTime = 0;
    QFFmpeg::SwrContextUPtr m_resampler;
    qint64 m_samplesProcessed = 0;
    qint64 m_endCompensationSample = std::numeric_limits<qint64>::min();
    qint32 m_sampleCompensationDelta = 0;
};

QT_END_NAMESPACE

#endif
