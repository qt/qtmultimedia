// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qgstdmabufvideobuffer_p.h"

#include <QtMultimedia/private/qvideotexturehelper_p.h>
#include <QtCore/qloggingcategory.h>

#include <gst/allocators/gstdmabuf.h>
#include <gst/video/gstvideometa.h>

#include <array>
#include <optional>

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(qLcGstDmaBufVideoBuffer, "qt.multimedia.gstreamer.dmabufvideobuffer")

using QtMultimediaPrivate::DmaBufImportMode;
using QtMultimediaPrivate::DmaBufPlane;
using QtMultimediaPrivate::DRMFormat;

static std::optional<DRMFormat> fourccFromGstVideoFormat(const GstVideoFormat format, int plane,
                                                         bool singleEGLImage)
{
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
    constexpr DRMFormat argb_fourcc = DRMFormat::ARGB8888;
    constexpr DRMFormat rgba_fourcc = DRMFormat::ABGR8888;
    constexpr DRMFormat rgb_fourcc = DRMFormat::BGR888;
    constexpr DRMFormat rg_fourcc = DRMFormat::GR88;
#else
    constexpr DRMFormat argb_fourcc = DRMFormat::BGRA8888;
    constexpr DRMFormat rgba_fourcc = DRMFormat::RGBA8888;
    constexpr DRMFormat rgb_fourcc = DRMFormat::RGB888;
    constexpr DRMFormat rg_fourcc = DRMFormat::RG88;
#endif

    switch (format) {
    case GST_VIDEO_FORMAT_RGB16:
    case GST_VIDEO_FORMAT_BGR16:
        return DRMFormat::RGB565;

    case GST_VIDEO_FORMAT_RGB:
    case GST_VIDEO_FORMAT_BGR:
        return rgb_fourcc;

    case GST_VIDEO_FORMAT_BGRx:
    case GST_VIDEO_FORMAT_BGRA:
        return argb_fourcc;

    case GST_VIDEO_FORMAT_AYUV:
        if (singleEGLImage)
            return DRMFormat::AYUV;
        [[fallthrough]];
    case GST_VIDEO_FORMAT_RGBx:
    case GST_VIDEO_FORMAT_RGBA:
    case GST_VIDEO_FORMAT_ARGB:
    case GST_VIDEO_FORMAT_xRGB:
    case GST_VIDEO_FORMAT_ABGR:
    case GST_VIDEO_FORMAT_xBGR:
        return rgba_fourcc;

    case GST_VIDEO_FORMAT_GRAY8:
        return DRMFormat::R8;

    case GST_VIDEO_FORMAT_YUY2:
        return DRMFormat::YUYV;

    case GST_VIDEO_FORMAT_UYVY:
        return DRMFormat::UYVY;

    case GST_VIDEO_FORMAT_GRAY16_LE:
    case GST_VIDEO_FORMAT_GRAY16_BE:
        if (singleEGLImage)
            return DRMFormat::R16;
        return rg_fourcc;

    case GST_VIDEO_FORMAT_NV12:
        if (singleEGLImage)
            return DRMFormat::NV12;
        [[fallthrough]];
    case GST_VIDEO_FORMAT_NV21:
        if (singleEGLImage)
            return DRMFormat::NV21;
        return plane == 0 ? DRMFormat::R8 : rg_fourcc;

    case GST_VIDEO_FORMAT_I420:
        if (singleEGLImage)
            return DRMFormat::YUV420;
        [[fallthrough]];
    case GST_VIDEO_FORMAT_YV12:
        if (singleEGLImage)
            return DRMFormat::YVU420;
        [[fallthrough]];
    case GST_VIDEO_FORMAT_Y41B:
        if (singleEGLImage)
            return DRMFormat::YUV411;
        [[fallthrough]];
    case GST_VIDEO_FORMAT_Y42B:
        if (singleEGLImage)
            return DRMFormat::YUV422;
        [[fallthrough]];
    case GST_VIDEO_FORMAT_Y444:
        if (singleEGLImage)
            return DRMFormat::YUV444;
        return DRMFormat::R8;

    case GST_VIDEO_FORMAT_BGR10A2_LE:
        return DRMFormat::BGRA1010102;

    case GST_VIDEO_FORMAT_P010_10LE:
    case GST_VIDEO_FORMAT_P010_10BE:
        if (singleEGLImage)
            return DRMFormat::P010;
        return plane == 0 ? DRMFormat::R16 : DRMFormat::RG1616;

    default:
        return std::nullopt;
    }
}

QGstDmaBufVideoBuffer::QGstDmaBufVideoBuffer(QSpan<const DmaBufPlane> planes,
                                             DmaBufImportMode importMode, QGstBufferHandle buffer,
                                             QVideoFrameFormat::PixelFormat format, QSize size)
    : QDmaBufVideoBuffer(format, size, planes, std::make_shared<QGstBufferHandle>(buffer),
                         importMode),
      m_buffer(std::move(buffer))
{
}

QGstDmaBufVideoBuffer::~QGstDmaBufVideoBuffer() = default;

std::unique_ptr<QGstDmaBufVideoBuffer>
QGstDmaBufVideoBuffer::create(QGstBufferHandle buffer, const QGstVideoInfo &videoInfo,
                              const QVideoFrameFormat &frameFormat)
{
    using namespace QtMultimediaPrivate;

    if (!buffer.isValid())
        return nullptr;

    GstBuffer *gstBuffer = buffer.get();
    if (!gst_is_dmabuf_memory(gst_buffer_peek_memory(gstBuffer, 0)))
        return nullptr;

    const GstVideoInfo &gstVideoInfo = videoInfo.gstVideoInfo;
    if (!gstVideoInfo.finfo)
        return nullptr;

    const GstVideoMeta *videoMeta = gst_buffer_get_video_meta(gstBuffer);
    GstVideoFormat format = videoMeta ? videoMeta->format : GST_VIDEO_INFO_FORMAT(&gstVideoInfo);
    if (format == GST_VIDEO_FORMAT_UNKNOWN)
        format = GST_VIDEO_INFO_FORMAT(&gstVideoInfo);

    const int nPlanes = videoMeta ? videoMeta->n_planes : GST_VIDEO_INFO_N_PLANES(&gstVideoInfo);
    const int nMemoryBlocks = gst_buffer_n_memory(gstBuffer);
    if (nPlanes < 1 || nPlanes > 4 || (nMemoryBlocks != 1 && nMemoryBlocks != nPlanes))
        return nullptr;

    const QVideoFrameFormat::PixelFormat pixelFormat = frameFormat.pixelFormat();
    const auto *desc = QVideoTextureHelper::textureDescription(pixelFormat);
    if (!desc || desc->nplanes != nPlanes)
        return nullptr;

    static const bool singleEGLImageEnv =
            qEnvironmentVariableIntValue("QT_GSTREAMER_FORCE_SINGLE_EGLIMAGE") != 0;
    const bool singleEGLImage =
            QVideoTextureHelper::forceGlTextureExternalOesIsSet() || singleEGLImageEnv;

    std::array<DmaBufPlane, 4> planes;
    std::array<int, 4> fds{ -1, -1, -1, -1 };
    for (int i = 0; i < nMemoryBlocks && i < 4; ++i)
        fds[i] = gst_dmabuf_memory_get_fd(gst_buffer_peek_memory(gstBuffer, i));

    const DRMModifier modifier = videoInfo.dmaDrmModifier ? DRMModifier{ *videoInfo.dmaDrmModifier }
                                                          : DmaBufFormatModifierInvalid;

    for (int plane = 0; plane < nPlanes; ++plane) {
        const std::optional<DRMFormat> fourcc =
                fourccFromGstVideoFormat(format, plane, singleEGLImage);
        if (!fourcc) {
            qCWarning(qLcGstDmaBufVideoBuffer)
                    << "Unsupported format for DMABuf:" << gst_video_format_to_string(format)
                    << "plane:" << plane << "singleEGLImage:" << singleEGLImage;
            return nullptr;
        }

        const int fd = (plane < nMemoryBlocks && fds[plane] >= 0) ? fds[plane] : fds[0];
        if (fd < 0)
            return nullptr;

        planes[plane].fd = fd;
        // videoMeta/videoInfo offset can be incorrect when each plane has a separate memory block
        if (nPlanes == nMemoryBlocks) {
            planes[plane].offset = 0;
        } else if (videoMeta) {
            planes[plane].offset = videoMeta->offset[plane];
        } else {
            planes[plane].offset = GST_VIDEO_INFO_PLANE_OFFSET(&gstVideoInfo, plane);
        }
        if (videoMeta)
            planes[plane].pitch = videoMeta->stride[plane];
        else
            planes[plane].pitch = GST_VIDEO_INFO_PLANE_STRIDE(&gstVideoInfo, plane);
        planes[plane].drmFormat = *fourcc;
        planes[plane].modifier = modifier;
    }

    const DmaBufImportMode importMode =
            singleEGLImage ? DmaBufImportMode::SingleImage : DmaBufImportMode::PerPlane;
    return std::unique_ptr<QGstDmaBufVideoBuffer>(
            new QGstDmaBufVideoBuffer(QSpan<const DmaBufPlane>(planes.data(), nPlanes), importMode,
                                      std::move(buffer), pixelFormat, frameFormat.frameSize()));
}

QT_END_NAMESPACE
