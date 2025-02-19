// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QFFMPEGVIDEOENCODER_P_H
#define QFFMPEGVIDEOENCODER_P_H

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

#include <QtFFmpegMediaPluginImpl/private/qffmpegencoderthread_p.h>
#include <QtFFmpegMediaPluginImpl/private/qffmpeg_p.h>
#include <QtFFmpegMediaPluginImpl/private/qffmpegvideoframeencoder_p.h>
#include <qvideoframe.h>
#include <queue>

QT_BEGIN_NAMESPACE

class QVideoFrameFormat;
class QMediaEncoderSettings;

namespace QFFmpeg {
class VideoFrameEncoder;

class VideoEncoder : public EncoderThread
{
public:
    VideoEncoder(RecordingEngine &recordingEngine, const QMediaEncoderSettings &settings,
                 const QVideoFrameFormat &format, std::optional<AVPixelFormat> hwFormat);
    ~VideoEncoder() override;

    void addFrame(const QVideoFrame &frame);

protected:
    bool checkIfCanPushFrame() const override;

private:
    struct FrameInfo
    {
        QVideoFrame frame;
        bool shouldAdjustTimeBase = false;
    };

    FrameInfo takeFrame();
    void retrievePackets();

    bool init() override;
    void cleanup() override;
    bool hasData() const override;
    void processOne() override;

    std::pair<qint64, qint64> frameTimeStamps(const QVideoFrame &frame) const;

private:
    QMediaEncoderSettings m_settings;
    VideoFrameEncoder::SourceParams m_sourceParams;
    std::queue<FrameInfo> m_videoFrameQueue;
    const size_t m_maxQueueSize = 10; // Arbitrarily chosen to limit memory usage (332 MB @ 4K)

    VideoFrameEncoderUPtr m_frameEncoder;
    qint64 m_baseTime = 0;
    bool m_shouldAdjustTimeBaseForNextFrame = true;
    qint64 m_lastFrameTime = 0;
};

} // namespace QFFmpeg

QT_END_NAMESPACE

#endif
