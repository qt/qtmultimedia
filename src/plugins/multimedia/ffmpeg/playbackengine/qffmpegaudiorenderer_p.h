// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QFFMPEGAUDIORENDERER_P_H
#define QFFMPEGAUDIORENDERER_P_H

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

#include "playbackengine/qffmpegrenderer_p.h"

#include "qaudiobuffer.h"

QT_BEGIN_NAMESPACE

class QAudioOutput;
class QAudioSink;

namespace QFFmpeg {
class Resampler;
};

namespace QFFmpeg {

class AudioRenderer : public Renderer
{
    Q_OBJECT
public:
    AudioRenderer(const TimeController &tc, QAudioOutput *output);

    void setOutput(QAudioOutput *output);

    ~AudioRenderer() override;

protected:
    RenderingResult renderInternal(Frame frame) override;

    void onPlaybackRateChanged() override;

    void freeOutput();

    void updateOutput(const Codec *codec);

    void initResempler(const Codec *codec);

    void onDeviceChanged();

    void updateVolume();

    void updateSynchronization(const Frame &currentFrame);

    std::chrono::microseconds currentBufferLoadingTime() const;

private:
    QPointer<QAudioOutput> m_output;
    std::unique_ptr<QAudioSink> m_sink;
    std::unique_ptr<Resampler> m_resampler;
    QAudioFormat m_format;

    QAudioBuffer m_bufferedData;
    qsizetype m_bufferWritten = 0;
    QIODevice *m_ioDevice = nullptr;

    bool m_deviceChanged = false;
    bool m_drained = false;
};

} // namespace QFFmpeg

QT_END_NAMESPACE

#endif // QFFMPEGAUDIORENDERER_P_H
