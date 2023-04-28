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
#include "qffmpeg_p.h"

QT_BEGIN_NAMESPACE

namespace QFFmpeg
{

class Codec;

class Resampler
{
public:
    Resampler(const Codec *codec, const QAudioFormat &outputFormat);
    ~Resampler();

    QAudioBuffer resample(const AVFrame *frame);
    qint64 samplesProcessed() const { return m_samplesProcessed; }
    void setSampleCompensation(qint32 delta, quint32 distance);
    bool isSampleCompensationActive() const;

private:
    QAudioFormat m_outputFormat;
    SwrContext *resampler = nullptr;
    qint64 m_samplesProcessed = 0;
    qint64 m_endCompensationSample = std::numeric_limits<qint64>::min();
};

}

QT_END_NAMESPACE

#endif
