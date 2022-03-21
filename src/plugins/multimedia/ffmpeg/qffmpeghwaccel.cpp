/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qffmpeghwaccel_p.h"
#if QT_CONFIG(vaapi)
#include "qffmpeghwaccel_vaapi_p.h"
#endif
#ifdef Q_OS_DARWIN
#include "qffmpeghwaccel_videotoolbox_p.h"
#endif
#include "qffmpeg_p.h"
#include "qffmpegvideobuffer_p.h"

#include <qdebug.h>

/* Infrastructure for HW acceleration goes into this file. */

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

// HW context initialization

// preferred order of HW accelerators to use
static const AVHWDeviceType preferredHardwareAccelerators[] = {
// Linux/Unix
#if defined(Q_OS_LINUX)
    AV_HWDEVICE_TYPE_VAAPI,
//    AV_HWDEVICE_TYPE_DRM,
#elif defined (Q_OS_WIN)
    AV_HWDEVICE_TYPE_D3D11VA,
#elif defined (Q_OS_DARWIN)
    AV_HWDEVICE_TYPE_VIDEOTOOLBOX,
#elif defined (Q_OS_ANDROID)
    AV_HWDEVICE_TYPE_MEDIACODEC,
#endif
    AV_HWDEVICE_TYPE_NONE
};

static AVBufferRef *loadHWContext(const AVHWDeviceType type)
{
    AVBufferRef *hwContext = nullptr;
    int ret = av_hwdevice_ctx_create(&hwContext, type, nullptr, nullptr, 0);
    qDebug() << "    Checking HW context:" << av_hwdevice_get_type_name(type);
    if (ret == 0) {
        qDebug() << "    Using above hw context.";
        return hwContext;
    }
    qDebug() << "    Could not create hw context:" << ret << strerror(-ret);
    return nullptr;
}

static AVBufferRef *hardwareContextForCodec(const AVCodec *codec)
{
    qDebug() << "Checking HW acceleration for decoder" << codec->name;

    // First try our preferred accelerators. Those are the ones where we can
    // set up a zero copy pipeline
    auto *preferred = preferredHardwareAccelerators;
    while (*preferred != AV_HWDEVICE_TYPE_NONE) {
        for (int i = 0;; ++i) {
            const AVCodecHWConfig *config = avcodec_get_hw_config(codec, i);
            if (!config)
                break;
            if (config->device_type == *preferred) {
                auto *hwContext = loadHWContext(config->device_type);
                if (hwContext)
                    return hwContext;
                break;
            }
        }
        ++preferred;
    }

    // Ok, let's see if we can get any HW acceleration at all. It'll still involve one buffer copy,
    // as we can't move the data into RHI textures without a CPU copy
    for (int i = 0;; ++i) {
        const AVCodecHWConfig *config = avcodec_get_hw_config(codec, i);
        if (!config)
            break;

        auto *hwContext = loadHWContext(config->device_type);
        if (hwContext)
            return hwContext;
    }
    qDebug() << "    No HW accelerators found, using SW decoding.";
    return nullptr;

}

static bool formatIsHWAccelerated(AVPixelFormat fmt)
{
    switch (fmt) {
    case AV_PIX_FMT_VAAPI:
    case AV_PIX_FMT_VDPAU:
    case AV_PIX_FMT_QSV:
    case AV_PIX_FMT_MMAL:
    case AV_PIX_FMT_D3D11:
    case AV_PIX_FMT_CUDA:
    case AV_PIX_FMT_DRM_PRIME:
    case AV_PIX_FMT_OPENCL:
    case AV_PIX_FMT_VIDEOTOOLBOX:
        return true;
    default:
        break;
    }
    return false;
}

// Used for the AVCodecContext::get_format callback
AVPixelFormat getFormat(AVCodecContext *s, const AVPixelFormat *fmt)
{
    Q_UNUSED(s);

    // check the pixel formats supported. We always want HW accelerated formats
    auto *f = fmt;
    while (*f != -1) {
        if (formatIsHWAccelerated(*f))
            return *f;
        ++f;
    }

    // prefer video formats we can handle directly
    f = fmt;
    while (*f != -1) {
        bool needsConversion = true;
        QFFmpegVideoBuffer::toQtPixelFormat(*f, &needsConversion);
        if (!needsConversion)
            return *f;
        ++f;
    }

    // take the native format, this will involve one additional format conversion on the CPU side
    return *fmt;
}

TextureConverter::Data::~Data()
{
    delete backend;
}



HWAccel::Data::~Data()
{
    if (hwDeviceContext)
        av_buffer_unref(&hwDeviceContext);
    if (hwFramesContext)
        av_buffer_unref(&hwFramesContext);
}


HWAccel::HWAccel(const AVCodec *codec)
{
    if (codec->type != AVMEDIA_TYPE_VIDEO)
        return;
    auto *ctx = hardwareContextForCodec(codec);
    if (!ctx)
        return;
    d = new Data;
    d->hwDeviceContext = ctx;
}

HWAccel::HWAccel(AVHWDeviceType deviceType)
{
    auto *ctx = loadHWContext(deviceType);
    if (!ctx)
        return;
    d = new Data;
    d->hwDeviceContext = ctx;
}

HWAccel::~HWAccel() = default;

AVPixelFormat HWAccel::format(AVFrame *frame)
{
    if (!frame->hw_frames_ctx)
        return AVPixelFormat(frame->format);

    auto *hwFramesContext = (AVHWFramesContext *)frame->hw_frames_ctx->data;
    Q_ASSERT(hwFramesContext);
    return AVPixelFormat(hwFramesContext->sw_format);
}

const AVHWDeviceType *HWAccel::preferredDeviceTypes()
{
    return preferredHardwareAccelerators;
}

AVHWDeviceContext *HWAccel::hwDeviceContext() const
{
    if (!d || !d->hwDeviceContext)
        return nullptr;
    return (AVHWDeviceContext *)d->hwDeviceContext->data;
}

AVPixelFormat HWAccel::hwFormat() const
{
    switch (deviceType()) {
    case AV_HWDEVICE_TYPE_VIDEOTOOLBOX:
        return AV_PIX_FMT_VIDEOTOOLBOX;
    case AV_HWDEVICE_TYPE_VAAPI:
        return AV_PIX_FMT_VAAPI;
    default:
        return AV_PIX_FMT_NONE;
    }
}

const AVCodec *HWAccel::hardwareEncoderForCodecId(AVCodecID id) const
{
    const char *codec = nullptr;
    switch (deviceType()) {
#ifdef Q_OS_DARWIN
    case AV_HWDEVICE_TYPE_VIDEOTOOLBOX:
        switch (id) {
        case AV_CODEC_ID_H264:
            codec = "h264_videotoolbox";
            break;
        case AV_CODEC_ID_HEVC:
            codec = "hevc_videotoolbox";
            break;
        case AV_CODEC_ID_PRORES:
            codec = "prores_videotoolbox";
            break;
        case AV_CODEC_ID_VP9:
            codec = "vp9_videotoolbox";
            break;
        default:
            break;
        }
        break;
#endif
    case AV_HWDEVICE_TYPE_VAAPI:
        switch (id) {
        case AV_CODEC_ID_H264:
            codec = "h264_vaapi";
            break;
        case AV_CODEC_ID_HEVC:
            codec = "hevc_vaapi";
            break;
        case AV_CODEC_ID_MJPEG:
            codec = "mjpeg_vaapi";
            break;
        case AV_CODEC_ID_MPEG2VIDEO:
            codec = "mpeg2_vaapi";
            break;
        case AV_CODEC_ID_VP8:
            codec = "vp8_vaapi";
            break;
        case AV_CODEC_ID_VP9:
            codec = "vp9_vaapi";
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }
    if (!codec)
        return nullptr;
    const AVCodec *c = avcodec_find_encoder_by_name(codec);
    qDebug() << "searching for HW codec" << codec << "got" << c;
    return c;
}

HWAccel HWAccel::findHardwareAccelForCodecID(AVCodecID id)
{
    auto *accels = preferredHardwareAccelerators;
    while (*accels != AV_HWDEVICE_TYPE_NONE) {
        auto accel = HWAccel(*accels);
        if (accel.hardwareEncoderForCodecId(id) != nullptr)
            return accel;
        ++accels;
    }
    return {};
}

AVHWDeviceType HWAccel::deviceType() const
{
    if (!d || !d->hwDeviceContext)
        return AV_HWDEVICE_TYPE_NONE;
    return hwDeviceContext()->type;
}

void HWAccel::createFramesContext(AVPixelFormat swFormat, const QSize &size)
{
    if (!d || !d->hwDeviceContext)
        return;
    d->hwFramesContext = av_hwframe_ctx_alloc(d->hwDeviceContext);
    auto *c = (AVHWFramesContext *)d->hwFramesContext->data;
    c->format = AV_PIX_FMT_VIDEOTOOLBOX;
    c->sw_format = swFormat;
    c->width = size.width();
    c->height = size.height();
    int err = av_hwframe_ctx_init(d->hwFramesContext);
    if (err < 0) {
        char str[AV_ERROR_MAX_STRING_SIZE];
        av_make_error_string(str, AV_ERROR_MAX_STRING_SIZE, err);
        qWarning() << "failed to init HW frame context" << err << str;
        return;
    }
}

AVHWFramesContext *HWAccel::hwFramesContext() const
{
    if (!d || !d->hwFramesContext)
        return nullptr;
    return (AVHWFramesContext *)d->hwFramesContext->data;
}


TextureConverter::TextureConverter(QRhi *rhi)
    : d(new Data)
{
    d->rhi = rhi;
}

TextureSet *TextureConverter::getTextures(AVFrame *frame)
{
    if (!frame || isNull())
        return nullptr;

    Q_ASSERT(frame->format == d->format);
    return d->backend->getTextures(frame);
}

void TextureConverter::updateBackend(AVPixelFormat fmt)
{
    d->backend = nullptr;
    if (!d->rhi)
        return;
    switch (fmt) {
#if QT_CONFIG(vaapi)
    case AV_PIX_FMT_VAAPI:
        d->backend = new VAAPITextureConverter(d->rhi);
        break;
#endif
#ifdef Q_OS_DARWIN
    case AV_PIX_FMT_VIDEOTOOLBOX:
        d->backend = new VideoToolBoxTextureConverter(d->rhi);
        break;
#endif
    default:
        break;
    }
    d->format = fmt;
}

} // namespace QFFmpeg

QT_END_NAMESPACE
