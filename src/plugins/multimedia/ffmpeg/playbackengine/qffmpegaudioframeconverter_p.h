// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QFFMPEGAUDIOFRAMECONVERTER_P_H
#define QFFMPEGAUDIOFRAMECONVERTER_P_H

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

#include <QtMultimedia/qaudiobuffer.h>

extern "C" {
#include <libavutil/frame.h>
}

#include <memory>

QT_BEGIN_NAMESPACE

class QFFmpegResampler;

namespace QFFmpeg {

struct AbstractAudioFrameConverter
{
    virtual ~AbstractAudioFrameConverter();
    virtual QAudioBuffer convert(AVFrame *) = 0;
};

struct Frame;

std::unique_ptr<QFFmpegResampler> createResampler(const Frame &frame,
                                                  const QAudioFormat &outputFormat);

std::unique_ptr<AbstractAudioFrameConverter>
makeTrivialAudioFrameConverter(const Frame &frame, QAudioFormat outputFormat, float playbackRate);

std::unique_ptr<AbstractAudioFrameConverter>
makePitchShiftingAudioFrameConverter(const Frame &frame, QAudioFormat outputFormat,
                                     float playbackRate);

} // namespace QFFmpeg

QT_END_NAMESPACE

#endif // QFFMPEGAUDIOFRAMECONVERTER_P_H
