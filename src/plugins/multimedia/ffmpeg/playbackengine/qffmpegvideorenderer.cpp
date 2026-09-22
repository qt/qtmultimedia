// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qffmpegvideorenderer_p.h"

#include "qffmpegvideobuffer_p.h"

#include <QtMultimedia/qvideosink.h>
#include <QtMultimedia/private/qvideoframe_p.h>

#ifdef Q_OS_WINDOWS
#  include "qffmpegvideosink_p.h"
#  include <QtMultimedia/private/qwindowsd3d11rhitextureimporter_p.h>
#  include <QtMultimedia/private/qwindowsd3d11videoprocessor_p.h>
#endif

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

VideoRenderer::VideoRenderer(const PlaybackEngineObjectID &id, const TimeController &tc,
                             QVideoSink *sink, const VideoTransformation &transform)
    : Renderer(id, tc), m_sink(sink), m_transform(transform)
{
}

void VideoRenderer::setOutput(QVideoSink *sink, bool cleanPrevSink)
{
    setOutputInternal(m_sink, sink, [=](QVideoSink *prev) {
        if (!prev)
            return;

        if (sink)
            sink->setVideoFrame(prev->videoFrame());

        if (cleanPrevSink)
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

    const auto codecContext = frame.codecContext();
    Q_ASSERT(codecContext);

#ifdef Q_OS_ANDROID
    // QTBUG-108446
    // In general case, just creation of frames context is not correct since
    //   frames may require additional specific data for hw contexts, so
    //   just setting of hw_frames_ctx is not enough.
    // TODO: investigate the case in order to remove or fix the code.
    if (codecContext->hwAccel() && !frame.avFrame()->hw_frames_ctx) {
        HWAccel *hwaccel = codecContext->hwAccel();
        AVFrame *avframe = frame.avFrame();
        if (!hwaccel->hwFramesContext())
            hwaccel->createFramesContext(AVPixelFormat(avframe->format),
                                         { avframe->width, avframe->height });

        if (hwaccel->hwFramesContext())
            avframe->hw_frames_ctx = av_buffer_ref(hwaccel->hwFramesContextAsBuffer());
    }
#endif

    const auto pixelAspectRatio = codecContext->pixelAspectRatio(frame.avFrame());

#ifdef Q_OS_WINDOWS
    // D3D12 and OpenGL can only import single-plane textures (see
    // QWindowsD3D11RhiTextureImporter::isRhiBackendSupported); an NV12 or P010 D3D11VA frame
    // needs to be converted to a single-plane format before it can be imported on those
    // backends. This has to be decided now rather than in QFFmpegVideoBuffer::mapTextures(),
    // since the QVideoFrameFormat below is fixed for the lifetime of the QVideoFrame.
    QVideoFrameFormat::PixelFormat pixelFormatOverride = QVideoFrameFormat::Format_Invalid;
    if (frame.avFrame()->format == AV_PIX_FMT_D3D11 && frame.avFrame()->hw_frames_ctx) {
        if (auto *ffmpegSink = qobject_cast<QFFmpegVideoSink *>(m_sink->platformVideoSink())) {
            ffmpegSink->withRhi([&](QRhi *rhi) {
                if (!rhi)
                    return;
                using namespace QtMultimediaPrivate;
                const auto swPixelFormat =
                        QFFmpegVideoBuffer::toQtPixelFormat(HWAccel::format(frame.avFrame()));
                if ((!QWindowsD3D11RhiTextureImporter::isRhiBackendSupported(*rhi)
                     || !QWindowsD3D11RhiTextureImporter::isPixelFormatSupportedByRhiBackend(
                             *rhi, swPixelFormat))
                    && QWindowsD3D11VideoProcessor::isPixelFormatSupported(swPixelFormat)) {
                    pixelFormatOverride = QVideoFrameFormat::Format_BGRA8888;
                }
            });
        }
    }
#endif

    auto buffer = std::make_unique<QFFmpegVideoBuffer>(frame.takeAVFrame(), pixelAspectRatio);
#ifdef Q_OS_WINDOWS
    if (pixelFormatOverride != QVideoFrameFormat::Format_Invalid)
        buffer->setPixelFormatOverride(pixelFormatOverride);
#endif
    QVideoFrameFormat format(buffer->size(), buffer->pixelFormat());
    format.setColorSpace(buffer->colorSpace());
    format.setColorTransfer(buffer->colorTransfer());
    format.setColorRange(buffer->colorRange());
    format.setMaxLuminance(buffer->maxNits());
    format.setRotation(m_transform.rotation);
    format.setMirrored(m_transform.mirroredHorizontallyAfterRotation);
    QVideoFrame videoFrame = QVideoFramePrivate::createFrame(std::move(buffer), format);
    videoFrame.setStartTime(frame.startTime().get());
    videoFrame.setEndTime(frame.endTime().get());
    m_sink->setVideoFrame(videoFrame);

    return {};
}

} // namespace QFFmpeg

QT_END_NAMESPACE

#include "moc_qffmpegvideorenderer_p.cpp"
