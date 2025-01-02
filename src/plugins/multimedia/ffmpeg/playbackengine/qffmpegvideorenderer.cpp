// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "playbackengine/qffmpegvideorenderer_p.h"
#include "qffmpegvideobuffer_p.h"
#include "qvideosink.h"

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

VideoRenderer::VideoRenderer(const TimeController &tc, QVideoSink *sink, QVideoFrame::RotationAngle rotationAngle)
    : Renderer(tc), m_sink(sink), m_rotationAngle(rotationAngle)
{
}

void VideoRenderer::setOutput(QVideoSink *sink, bool cleanPrevSink)
{
    setOutputInternal(m_sink, sink, [cleanPrevSink](QVideoSink *prev) {
        if (prev && cleanPrevSink)
            prev->setVideoFrame({});
    });
}

VideoRenderer::RenderingResult VideoRenderer::renderInternal(Frame frame)
{
    if (!m_sink)
        return {};

    if (!frame.isValid()) {
        m_sink->setVideoFrame({});
        return {};
    }

    //        qCDebug(qLcVideoRenderer) << "RHI:" << accel.isNull() << accel.rhi() << sink->rhi();

    const auto codec = frame.codec();
    Q_ASSERT(codec);

#ifdef Q_OS_ANDROID
    // QTBUG-108446
    // In general case, just creation of frames context is not correct since
    //   frames may require additional specific data for hw contexts, so
    //   just setting of hw_frames_ctx is not enough.
    // TODO: investigate the case in order to remove or fix the code.
    if (codec->hwAccel() && !frame.avFrame()->hw_frames_ctx) {
        HWAccel *hwaccel = codec->hwAccel();
        AVFrame *avframe = frame.avFrame();
        if (!hwaccel->hwFramesContext())
            hwaccel->createFramesContext(AVPixelFormat(avframe->format),
                                         { avframe->width, avframe->height });

        if (hwaccel->hwFramesContext())
            avframe->hw_frames_ctx = av_buffer_ref(hwaccel->hwFramesContextAsBuffer());
    }
#endif

    const auto pixelAspectRatio = codec->pixelAspectRatio(frame.avFrame());
    auto buffer = std::make_unique<QFFmpegVideoBuffer>(frame.takeAVFrame(), pixelAspectRatio);
    QVideoFrameFormat format(buffer->size(), buffer->pixelFormat());
    format.setColorSpace(buffer->colorSpace());
    format.setColorTransfer(buffer->colorTransfer());
    format.setColorRange(buffer->colorRange());
    format.setMaxLuminance(buffer->maxNits());
    QVideoFrame videoFrame(buffer.release(), format);
    videoFrame.setStartTime(frame.pts());
    videoFrame.setEndTime(frame.end());
    videoFrame.setRotationAngle(m_rotationAngle);
    m_sink->setVideoFrame(videoFrame);

    return {};
}

} // namespace QFFmpeg

QT_END_NAMESPACE

#include "moc_qffmpegvideorenderer_p.cpp"
