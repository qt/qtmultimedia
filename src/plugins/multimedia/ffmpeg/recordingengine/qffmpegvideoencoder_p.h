// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QFFMPEGVIDEOENCODER_P_H
#define QFFMPEGVIDEOENCODER_P_H

#include "qffmpegencoderthread_p.h"
#include "qffmpeg_p.h"
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

    bool isValid() const;

    void addFrame(const QVideoFrame &frame);

    void setPaused(bool b) override
    {
        EncoderThread::setPaused(b);
        if (b)
            m_baseTime.storeRelease(-1);
    }

private:
    QVideoFrame takeFrame();
    void retrievePackets();

    void init() override;
    void cleanup() override;
    bool hasData() const override;
    void processOne() override;

private:
    std::queue<QVideoFrame> m_videoFrameQueue;
    const size_t m_maxQueueSize = 10; // Arbitrarily chosen to limit memory usage (332 MB @ 4K)

    std::unique_ptr<VideoFrameEncoder> m_frameEncoder;
    QAtomicInteger<qint64> m_baseTime = std::numeric_limits<qint64>::min();
    qint64 m_lastFrameTime = 0;
};

} // namespace QFFmpeg

QT_END_NAMESPACE

#endif
