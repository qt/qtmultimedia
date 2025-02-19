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

#include <QtFFmpegMediaPluginImpl/private/qffmpegrenderer_p.h>

#include "qaudiobuffer.h"

QT_BEGIN_NAMESPACE

class QAudioOutput;
class QAudioBufferOutput;
class QAudioSink;
class QFFmpegResampler;

namespace QFFmpeg {

struct AbstactAudioFrameConverter
{
    virtual ~AbstactAudioFrameConverter();
    virtual QAudioBuffer convert(AVFrame *) = 0;
};

class AudioRenderer : public Renderer
{
    Q_OBJECT
public:
    AudioRenderer(const TimeController &tc, QAudioOutput *output, QAudioBufferOutput *bufferOutput);

    void setOutput(QAudioOutput *output);

    void setOutput(QAudioBufferOutput *bufferOutput);

    ~AudioRenderer() override;

protected:
    using Microseconds = std::chrono::microseconds;
    struct SynchronizationStamp
    {
        QAudio::State audioSinkState = QAudio::IdleState;
        qsizetype audioSinkBytesFree = 0;
        qsizetype bufferBytesWritten = 0;
        TimePoint timePoint = TimePoint::max();
    };

    struct BufferLoadingInfo
    {
        enum Type { Low, Moderate, High };
        Type type = Moderate;
        TimePoint timePoint = TimePoint::max();
        Microseconds delay = Microseconds(0);
    };

    struct AudioTimings
    {
        Microseconds actualBufferDuration = Microseconds(0);
        Microseconds maxSoundDelay = Microseconds(0);
        Microseconds minSoundDelay = Microseconds(0);
    };

    struct BufferedDataWithOffset
    {
        QAudioBuffer buffer;
        qsizetype offset = 0;

        bool isValid() const { return buffer.isValid(); }
        qsizetype size() const { return buffer.byteCount() - offset; }
        const char *data() const { return buffer.constData<char>() + offset; }
    };

    RenderingResult renderInternal(Frame frame) override;

    RenderingResult pushFrameToOutput(const Frame &frame);

    void pushFrameToBufferOutput(const Frame &frame);

    void onPlaybackRateChanged() override;

    std::chrono::milliseconds timerInterval() const override;

    void onPauseChanged() override;

    void freeOutput();

    void updateOutputs(const Frame &frame);

    void initAudioFrameConverter(const Frame &frame);

    void onDeviceChanged();

    void updateVolume();

    void updateSynchronization(const SynchronizationStamp &stamp, const Frame &frame);

    Microseconds bufferLoadingTime(const SynchronizationStamp &syncStamp) const;

    void onAudioSinkStateChanged(QAudio::State state);

    Microseconds durationForBytes(qsizetype bytes) const;

private:
    QPointer<QAudioOutput> m_output;
    QPointer<QAudioBufferOutput> m_bufferOutput;
    std::unique_ptr<QAudioSink> m_sink;
    AudioTimings m_timings;
    BufferLoadingInfo m_bufferLoadingInfo;
    std::unique_ptr<QFFmpegResampler> m_bufferOutputResampler;
    QAudioFormat m_sinkFormat;

    BufferedDataWithOffset m_bufferedData;
    QIODevice *m_ioDevice = nullptr;

    bool m_lastFramePushDone = true;

    bool m_deviceChanged = false;
    bool m_bufferOutputChanged = false;
    bool m_drained = false;
    bool m_firstFrameToSink = true;

    // pitch compensation
    bool m_pitchCompensation = false; // TODO: we need an API to enable pitch compensation
    std::unique_ptr<AbstactAudioFrameConverter> m_audioFrameConverter;
};

} // namespace QFFmpeg

QT_END_NAMESPACE

#endif // QFFMPEGAUDIORENDERER_P_H
