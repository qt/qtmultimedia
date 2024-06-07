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

#include "qvideotexturehelper_p.h"
#include "qvideoframe.h"
#ifdef Q_OS_ANDROID
#include <private/qandroidvideooutput_p.h>
#endif

#include <qpainter.h>
#include <qloggingcategory.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(qLcVideoTextureHelper, "qt.multimedia.video.texturehelper")

namespace QVideoTextureHelper
{

static const TextureDescription descriptions[QVideoFrameFormat::NPixelFormats] = {
    //  Format_Invalid
    { 0, 0,
      [](int, int) { return 0; },
     { QRhiTexture::UnknownFormat, QRhiTexture::UnknownFormat, QRhiTexture::UnknownFormat},
     { { 1, 1 }, { 1, 1 }, { 1, 1 } }
    },
    // Format_ARGB8888
    { 1, 4,
        [](int stride, int height) { return stride*height; },
        { QRhiTexture::RGBA8, QRhiTexture::UnknownFormat, QRhiTexture::UnknownFormat },
        { { 1, 1 }, { 1, 1 }, { 1, 1 } }
    },
    // Format_ARGB8888_Premultiplied
    { 1, 4,
        [](int stride, int height) { return stride*height; },
        { QRhiTexture::RGBA8, QRhiTexture::UnknownFormat, QRhiTexture::UnknownFormat },
        { { 1, 1 }, { 1, 1 }, { 1, 1 } }
    },
    // Format_XRGB8888
    { 1, 4,
        [](int stride, int height) { return stride*height; },
        { QRhiTexture::RGBA8, QRhiTexture::UnknownFormat, QRhiTexture::UnknownFormat },
        { { 1, 1 }, { 1, 1 }, { 1, 1 } }
    },
    // Format_BGRA8888
    { 1, 4,
        [](int stride, int height) { return stride*height; },
        { QRhiTexture::BGRA8, QRhiTexture::UnknownFormat, QRhiTexture::UnknownFormat },
        { { 1, 1 }, { 1, 1 }, { 1, 1 } }
    },
    // Format_BGRA8888_Premultiplied
    { 1, 4,
        [](int stride, int height) { return stride*height; },
        { QRhiTexture::BGRA8, QRhiTexture::UnknownFormat, QRhiTexture::UnknownFormat },
        { { 1, 1 }, { 1, 1 }, { 1, 1 } }
    },
    // Format_BGRX8888
    { 1, 4,
        [](int stride, int height) { return stride*height; },
        { QRhiTexture::BGRA8, QRhiTexture::UnknownFormat, QRhiTexture::UnknownFormat },
        { { 1, 1 }, { 1, 1 }, { 1, 1 } }
    },
    // Format_ABGR8888
    { 1, 4,
        [](int stride, int height) { return stride*height; },
        { QRhiTexture::RGBA8, QRhiTexture::UnknownFormat, QRhiTexture::UnknownFormat },
        { { 1, 1 }, { 1, 1 }, { 1, 1 } }
    },
    // Format_XBGR8888
    { 1, 4,
        [](int stride, int height) { return stride*height; },
        { QRhiTexture::RGBA8, QRhiTexture::UnknownFormat, QRhiTexture::UnknownFormat },
        { { 1, 1 }, { 1, 1 }, { 1, 1 } }
    },
    // Format_RGBA8888
    { 1, 4,
        [](int stride, int height) { return stride*height; },
        { QRhiTexture::RGBA8, QRhiTexture::UnknownFormat, QRhiTexture::UnknownFormat },
        { { 1, 1 }, { 1, 1 }, { 1, 1 } }
    },
    // Format_RGBX8888
    { 1, 4,
        [](int stride, int height) { return stride*height; },
        { QRhiTexture::RGBA8, QRhiTexture::UnknownFormat, QRhiTexture::UnknownFormat },
        { { 1, 1 }, { 1, 1 }, { 1, 1 } }
    },
    // Format_AYUV
    { 1, 4,
      [](int stride, int height) { return stride*height; },
     { QRhiTexture::RGBA8, QRhiTexture::UnknownFormat, QRhiTexture::UnknownFormat },
     { { 1, 1 }, { 1, 1 }, { 1, 1 } }
    },
    // Format_AYUV_Premultiplied
    { 1, 4,
        [](int stride, int height) { return stride*height; },
        { QRhiTexture::RGBA8, QRhiTexture::UnknownFormat, QRhiTexture::UnknownFormat },
        { { 1, 1 }, { 1, 1 }, { 1, 1 } }
    },
    // Format_YUV420P
    { 3, 1,
      [](int stride, int height) { return stride * ((height * 3 / 2 + 1) & ~1); },
     { QRhiTexture::R8, QRhiTexture::R8, QRhiTexture::R8 },
     { { 1, 1 }, { 2, 2 }, { 2, 2 } }
    },
     // Format_YUV422P
    { 3, 1,
      [](int stride, int height) { return stride * ((height * 3 / 2 + 1) & ~1); },
     { QRhiTexture::R8, QRhiTexture::R8, QRhiTexture::R8 },
     { { 1, 1 }, { 2, 1 }, { 2, 1 } }
    },
     // Format_YV12
    { 3, 1,
      [](int stride, int height) { return stride * ((height * 3 / 2 + 1) & ~1); },
     { QRhiTexture::R8, QRhiTexture::R8, QRhiTexture::R8 },
     { { 1, 1 }, { 2, 2 }, { 2, 2 } }
    },
    // Format_UYVY
    { 1, 2,
      [](int stride, int height) { return stride*height; },
     { QRhiTexture::RGBA8, QRhiTexture::UnknownFormat, QRhiTexture::UnknownFormat },
     { { 2, 1 }, { 1, 1 }, { 1, 1 } }
    },
    // Format_YUYV
    { 1, 2,
      [](int stride, int height) { return stride*height; },
     { QRhiTexture::RGBA8, QRhiTexture::UnknownFormat, QRhiTexture::UnknownFormat },
     { { 2, 1 }, { 1, 1 }, { 1, 1 } }
    },
    // Format_NV12
    { 2, 1,
      [](int stride, int height) { return stride * ((height * 3 / 2 + 1) & ~1); },
     { QRhiTexture::R8, QRhiTexture::RG8, QRhiTexture::UnknownFormat },
     { { 1, 1 }, { 2, 2 }, { 1, 1 } }
    },
    // Format_NV21
    { 2, 1,
      [](int stride, int height) { return stride * ((height * 3 / 2 + 1) & ~1); },
     { QRhiTexture::R8, QRhiTexture::RG8, QRhiTexture::UnknownFormat },
     { { 1, 1 }, { 2, 2 }, { 1, 1 } }
    },
    // Format_IMC1
    { 3, 1,
      [](int stride, int height) {
          // IMC1 requires that U and V components are aligned on a multiple of 16 lines
          int h = (height + 15) & ~15;
          h += 2*(((h/2) + 15) & ~15);
          return stride * h;
      },
     { QRhiTexture::R8, QRhiTexture::R8, QRhiTexture::R8 },
     { { 1, 1 }, { 2, 2 }, { 2, 2 } }
    },
    // Format_IMC2
    { 2, 1,
      [](int stride, int height) { return 2*stride*height; },
     { QRhiTexture::R8, QRhiTexture::R8, QRhiTexture::UnknownFormat },
     { { 1, 1 }, { 1, 2 }, { 1, 1 } }
    },
    // Format_IMC3
    { 3, 1,
      [](int stride, int height) {
          // IMC3 requires that U and V components are aligned on a multiple of 16 lines
          int h = (height + 15) & ~15;
          h += 2*(((h/2) + 15) & ~15);
          return stride * h;
      },
     { QRhiTexture::R8, QRhiTexture::R8, QRhiTexture::R8 },
     { { 1, 1 }, { 2, 2 }, { 2, 2 } }
    },
    // Format_IMC4
    { 2, 1,
      [](int stride, int height) { return 2*stride*height; },
     { QRhiTexture::R8, QRhiTexture::R8, QRhiTexture::UnknownFormat },
     { { 1, 1 }, { 1, 2 }, { 1, 1 } }
    },
    // Format_Y8
    { 1, 1,
      [](int stride, int height) { return stride*height; },
     { QRhiTexture::R8, QRhiTexture::UnknownFormat, QRhiTexture::UnknownFormat },
     { { 1, 1 }, { 1, 1 }, { 1, 1 } }
    },
    // Format_Y16
    { 1, 2,
      [](int stride, int height) { return stride*height; },
     { QRhiTexture::R16, QRhiTexture::UnknownFormat, QRhiTexture::UnknownFormat },
     { { 1, 1 }, { 1, 1 }, { 1, 1 } }
    },
    // Format_P010
    { 2, 2,
      [](int stride, int height) { return stride * ((height * 3 / 2 + 1) & ~1); },
     { QRhiTexture::R16, QRhiTexture::RG16, QRhiTexture::UnknownFormat },
     { { 1, 1 }, { 2, 2 }, { 1, 1 } }
    },
    // Format_P016
    { 2, 2,
      [](int stride, int height) { return stride * ((height * 3 / 2 + 1) & ~1); },
     { QRhiTexture::R16, QRhiTexture::RG16, QRhiTexture::UnknownFormat },
     { { 1, 1 }, { 2, 2 }, { 1, 1 } }
    },
    // Format_SamplerExternalOES
    {
        1, 0,
        [](int, int) { return 0; },
        { QRhiTexture::RGBA8, QRhiTexture::UnknownFormat, QRhiTexture::UnknownFormat },
        { { 1, 1 }, { 1, 1 }, { 1, 1 } }
    },
    // Format_Jpeg
    { 1, 0,
      [](int, int) { return 0; },
     { QRhiTexture::UnknownFormat, QRhiTexture::UnknownFormat, QRhiTexture::UnknownFormat },
     { { 1, 1 }, { 1, 1 }, { 1, 1 } }
    },
    // Format_SamplerRect
    {
        1, 0,
        [](int, int) { return 0; },
        { QRhiTexture::BGRA8, QRhiTexture::UnknownFormat, QRhiTexture::UnknownFormat },
        { { 1, 1 }, { 1, 1 }, { 1, 1 } }
    }
};

const TextureDescription *textureDescription(QVideoFrameFormat::PixelFormat format)
{
    return descriptions + format;
}

QString vertexShaderFileName(QVideoFrameFormat::PixelFormat format)
{
    Q_UNUSED(format);

#if 1//def Q_OS_ANDROID
    if (format == QVideoFrameFormat::Format_SamplerExternalOES)
        return QStringLiteral(":/qt-project.org/multimedia/shaders/externalsampler.vert.qsb");
#endif
#if 1//def Q_OS_MACOS
    if (format == QVideoFrameFormat::Format_SamplerRect)
        return QStringLiteral(":/qt-project.org/multimedia/shaders/rectsampler.vert.qsb");
#endif

    return QStringLiteral(":/qt-project.org/multimedia/shaders/vertex.vert.qsb");
}

QString fragmentShaderFileName(QVideoFrameFormat::PixelFormat format)
{
    switch (format) {
    case QVideoFrameFormat::Format_Y8:
    case QVideoFrameFormat::Format_Y16:
        return QStringLiteral(":/qt-project.org/multimedia/shaders/y.frag.qsb");
    case QVideoFrameFormat::Format_AYUV:
    case QVideoFrameFormat::Format_AYUV_Premultiplied:
        return QStringLiteral(":/qt-project.org/multimedia/shaders/ayuv.frag.qsb");
    case QVideoFrameFormat::Format_ARGB8888:
    case QVideoFrameFormat::Format_ARGB8888_Premultiplied:
    case QVideoFrameFormat::Format_XRGB8888:
        return QStringLiteral(":/qt-project.org/multimedia/shaders/argb.frag.qsb");
    case QVideoFrameFormat::Format_ABGR8888:
    case QVideoFrameFormat::Format_XBGR8888:
        return QStringLiteral(":/qt-project.org/multimedia/shaders/abgr.frag.qsb");
    case QVideoFrameFormat::Format_RGBA8888:
    case QVideoFrameFormat::Format_RGBX8888:
    case QVideoFrameFormat::Format_BGRA8888:
    case QVideoFrameFormat::Format_BGRA8888_Premultiplied:
    case QVideoFrameFormat::Format_BGRX8888:
        return QStringLiteral(":/qt-project.org/multimedia/shaders/rgba.frag.qsb");
    case QVideoFrameFormat::Format_YUV420P:
    case QVideoFrameFormat::Format_YUV422P:
    case QVideoFrameFormat::Format_IMC3:
        return QStringLiteral(":/qt-project.org/multimedia/shaders/yuv_triplanar.frag.qsb");
    case QVideoFrameFormat::Format_YV12:
    case QVideoFrameFormat::Format_IMC1:
        return QStringLiteral(":/qt-project.org/multimedia/shaders/yvu_triplanar.frag.qsb");
    case QVideoFrameFormat::Format_IMC2:
        return QStringLiteral(":/qt-project.org/multimedia/shaders/imc2.frag.qsb");
    case QVideoFrameFormat::Format_IMC4:
        return QStringLiteral(":/qt-project.org/multimedia/shaders/imc4.frag.qsb");
    case QVideoFrameFormat::Format_UYVY:
        return QStringLiteral(":/qt-project.org/multimedia/shaders/uyvy.frag.qsb");
    case QVideoFrameFormat::Format_YUYV:
        return QStringLiteral(":/qt-project.org/multimedia/shaders/yuyv.frag.qsb");
    case QVideoFrameFormat::Format_NV12:
    case QVideoFrameFormat::Format_P010:
    case QVideoFrameFormat::Format_P016:
        // P010/P016 have the same layout as NV12, just 16 instead of 8 bits per pixel
        return QStringLiteral(":/qt-project.org/multimedia/shaders/nv12.frag.qsb");
    case QVideoFrameFormat::Format_NV21:
        return QStringLiteral(":/qt-project.org/multimedia/shaders/nv21.frag.qsb");
    case QVideoFrameFormat::Format_SamplerExternalOES:
#if 1//def Q_OS_ANDROID
        return QStringLiteral(":/qt-project.org/multimedia/shaders/externalsampler.frag.qsb");
#endif
    case QVideoFrameFormat::Format_SamplerRect:
#if 1//def Q_OS_MACOS
        return QStringLiteral(":/qt-project.org/multimedia/shaders/rectsampler_bgra.frag.qsb");
#endif
        // fallthrough
    case QVideoFrameFormat::Format_Invalid:
    case QVideoFrameFormat::Format_Jpeg:
    default:
        return QString();
    }
}


static QMatrix4x4 colorMatrix(QVideoFrameFormat::YCbCrColorSpace colorSpace)
{
    switch (colorSpace) {
    case QVideoFrameFormat::YCbCr_JPEG:
        return QMatrix4x4(
            1.0f,  0.000f,  1.402f, -0.701f,
            1.0f, -0.344f, -0.714f,  0.529f,
            1.0f,  1.772f,  0.000f, -0.886f,
            0.0f,  0.000f,  0.000f,  1.0000f);
    case QVideoFrameFormat::YCbCr_BT709:
    case QVideoFrameFormat::YCbCr_xvYCC709:
        return QMatrix4x4(
            1.1644f,  0.000f,  1.7928f, -0.9731f,
            1.1644f, -0.2132f, -0.5329f,  0.3015f,
            1.1644f,  2.1124f,  0.000f, -1.1335f,
            0.0f,    0.000f,  0.000f,  1.0000f);
    case QVideoFrameFormat::YCbCr_BT2020:
        return QMatrix4x4(
            1.1644f,  0.000f,  1.6787f, -0.9158f,
            1.1644f, -0.1874f, -0.6511f,  0.3478f,
            1.1644f,  2.1418f,  0.000f, -1.1483f,
            0.0f,  0.000f,  0.000f,  1.0000f);
    default: //BT 601:
        return QMatrix4x4(
            1.164f,  0.000f,  1.596f, -0.8708f,
            1.164f, -0.392f, -0.813f,  0.5296f,
            1.164f,  2.017f,  0.000f, -1.081f,
            0.0f,    0.000f,  0.000f,  1.0000f);
    }
}

#if 0
static QMatrix4x4 yuvColorCorrectionMatrix(float brightness, float contrast, float hue, float saturation)
{
    // Color correction in YUV space is done as follows:

    // The formulas assumes values in range 0-255, and a blackpoint of Y=16, whitepoint of Y=235
    //
    // Bightness: b
    // Contrast: c
    // Hue: h
    // Saturation: s
    //
    // Y' = (Y - 16)*c + b + 16
    // U' = ((U - 128)*cos(h) + (V - 128)*sin(h))*c*s + 128
    // V' = ((V - 128)*cos(h) - (U - 128)*sin(h))*c*s + 128
    //
    // For normalized YUV values (0-1 range) as we have them in the pixel shader, this translates to:
    //
    // Y' = (Y - .0625)*c + b + .0625
    // U' = ((U - .5)*cos(h) + (V - .5)*sin(h))*c*s + .5
    // V' = ((V - .5)*cos(h) - (U - .5)*sin(h))*c*s + .5
    //
    // The values need to be clamped to 0-1 after the correction and before converting to RGB
    // The transformation can be encoded in a 4x4 matrix assuming we have an A component of 1

    float chcs = cos(hue)*contrast*saturation;
    float shcs = sin(hue)*contrast*saturation;
    return QMatrix4x4(contrast,     0,    0, .0625*(1 - contrast) + brightness,
                      0,         chcs, shcs,              .5*(1 - chcs - shcs),
                      0,        -shcs, chcs,              .5*(1 + shcs - chcs),
                      0,            0,    0,                                1);
}
#endif

void updateUniformData(QByteArray *dst, const QVideoFrameFormat &format, const QVideoFrame &frame, const QMatrix4x4 &transform, float opacity)
{
#ifndef Q_OS_ANDROID
    Q_UNUSED(frame);
#endif

    QMatrix4x4 cmat;
    switch (format.pixelFormat()) {
    case QVideoFrameFormat::Format_Invalid:
    case QVideoFrameFormat::Format_Jpeg:
        return;

    case QVideoFrameFormat::Format_ARGB8888:
    case QVideoFrameFormat::Format_ARGB8888_Premultiplied:
    case QVideoFrameFormat::Format_XRGB8888:
    case QVideoFrameFormat::Format_BGRA8888:
    case QVideoFrameFormat::Format_BGRA8888_Premultiplied:
    case QVideoFrameFormat::Format_BGRX8888:
    case QVideoFrameFormat::Format_ABGR8888:
    case QVideoFrameFormat::Format_XBGR8888:
    case QVideoFrameFormat::Format_RGBA8888:
    case QVideoFrameFormat::Format_RGBX8888:

    case QVideoFrameFormat::Format_Y8:
    case QVideoFrameFormat::Format_Y16:
        break;
    case QVideoFrameFormat::Format_IMC1:
    case QVideoFrameFormat::Format_IMC2:
    case QVideoFrameFormat::Format_IMC3:
    case QVideoFrameFormat::Format_IMC4:
    case QVideoFrameFormat::Format_AYUV:
    case QVideoFrameFormat::Format_AYUV_Premultiplied:
    case QVideoFrameFormat::Format_YUV420P:
    case QVideoFrameFormat::Format_YUV422P:
    case QVideoFrameFormat::Format_YV12:
    case QVideoFrameFormat::Format_UYVY:
    case QVideoFrameFormat::Format_YUYV:
    case QVideoFrameFormat::Format_NV12:
    case QVideoFrameFormat::Format_NV21:
    case QVideoFrameFormat::Format_P010:
    case QVideoFrameFormat::Format_P016:
        cmat = colorMatrix(format.yCbCrColorSpace());
        break;
    case QVideoFrameFormat::Format_SamplerExternalOES:
#ifdef Q_OS_ANDROID
    {
        // get Android specific transform for the externalsampler texture
        if (auto *buffer = static_cast<AndroidTextureVideoBuffer *>(frame.videoBuffer()))
            cmat = buffer->externalTextureMatrix();
    }
#endif
        break;
    case QVideoFrameFormat::Format_SamplerRect:
    {
        // Similarly to SamplerExternalOES, the "color matrix" is used here to
        // transform the texture coordinates. OpenGL texture rectangles expect
        // non-normalized UVs, so apply a scale to have the fragment shader see
        // UVs in range [width,height] instead of [0,1].
        const QSize videoSize = frame.size();
        cmat.scale(videoSize.width(), videoSize.height());
    }
        break;
    }
    // { matrix, colorMatrix, opacity, width }
    Q_ASSERT(dst->size() >= 64 + 64 + 4 + 4);
    char *data = dst->data();
    memcpy(data, transform.constData(), 64);
    memcpy(data + 64, cmat.constData(), 64);
    memcpy(data + 64 + 64, &opacity, 4);
    float width = format.frameWidth();
    memcpy(data + 64 + 64 + 4, &width, 4);
}

int updateRhiTextures(QVideoFrame frame, QRhi *rhi, QRhiResourceUpdateBatch *resourceUpdates, QRhiTexture **textures)
{
    QVideoFrameFormat fmt = frame.surfaceFormat();
    QVideoFrameFormat::PixelFormat pixelFormat = fmt.pixelFormat();
    QSize size = fmt.frameSize();

    const TextureDescription *description = descriptions + pixelFormat;

    QSize planeSizes[TextureDescription::maxPlanes];
    for (int plane = 0; plane < description->nplanes; ++plane)
        planeSizes[plane] = QSize(size.width()/description->sizeScale[plane].x, size.height()/description->sizeScale[plane].y);

    if (frame.handleType() == QVideoFrame::RhiTextureHandle) {
        QRhiTexture::Flags textureFlags = {};
        if (pixelFormat == QVideoFrameFormat::Format_SamplerExternalOES) {
#ifdef Q_OS_ANDROID
            if (rhi->backend() == QRhi::OpenGLES2)
                textureFlags |= QRhiTexture::ExternalOES;
#endif
        }
        if (pixelFormat == QVideoFrameFormat::Format_SamplerRect) {
#ifdef Q_OS_MACOS
            if (rhi->backend() == QRhi::OpenGLES2)
                textureFlags |= QRhiTexture::TextureRectangleGL;
#endif
        }

        quint64 textureHandles[TextureDescription::maxPlanes] = {};
        bool textureHandlesOK = true;
        for (int plane = 0; plane < description->nplanes; ++plane) {
            quint64 handle = frame.textureHandle(plane);
            textureHandles[plane] = handle;
            textureHandlesOK &= handle > 0;
        }

        if (textureHandlesOK) {
            for (int plane = 0; plane < description->nplanes; ++plane) {
                textures[plane] = rhi->newTexture(description->textureFormat[plane], planeSizes[plane], 1, textureFlags);
                if (!textures[plane]->createFrom({ textureHandles[plane], 0}))
                    qWarning("Failed to initialize QRhiTexture wrapper for native texture object %llu", textureHandles[plane]);
            }
            return description->nplanes;
        } else {
            qCDebug(qLcVideoTextureHelper) << "Incorrect texture handle from QVideoFrame, trying to map and upload texture";
        }
    }

    // need to upload textures
    bool mapped = frame.map(QVideoFrame::ReadOnly);
    if (!mapped) {
        qWarning() << "could not map data of QVideoFrame for upload";
        return 0;
    }

    Q_ASSERT(frame.planeCount() == description->nplanes);
    for (int plane = 0; plane < description->nplanes; ++plane) {

        bool needsRebuild = !textures[plane] || textures[plane]->pixelSize() != planeSizes[plane];
        if (!textures[plane])
            textures[plane] = rhi->newTexture(description->textureFormat[plane], planeSizes[plane], 1, {});

        if (needsRebuild) {
            textures[plane]->setPixelSize(planeSizes[plane]);
            bool created = textures[plane]->create();
            if (!created) {
                qWarning("Failed to create texture (size %dx%d)", planeSizes[plane].width(), planeSizes[plane].height());
                return 0;
            }
        }

        auto data = QByteArray::fromRawData((const char *)frame.bits(plane), frame.mappedBytes(plane));
        QRhiTextureSubresourceUploadDescription subresDesc(data);
        subresDesc.setDataStride(frame.bytesPerLine(plane));
        QRhiTextureUploadEntry entry(0, 0, subresDesc);
        QRhiTextureUploadDescription desc({ entry });
        resourceUpdates->uploadTexture(textures[plane], desc);
    }
    return description->nplanes;
}

bool SubtitleLayout::update(const QSize &frameSize, QString text)
{
    text.replace(QLatin1Char('\n'), QChar::LineSeparator);
    if (layout.text() == text && videoSize == frameSize)
        return false;

    videoSize = frameSize;
    QFont font;
    // 0.045 - based on this https://www.md-subs.com/saa-subtitle-font-size
    qreal fontSize = frameSize.height() * 0.045;
    font.setPointSize(fontSize);

    layout.setText(text);
    if (text.isEmpty()) {
        bounds = {};
        return true;
    }
    layout.setFont(font);
    QTextOption option;
    option.setUseDesignMetrics(true);
    option.setAlignment(Qt::AlignCenter);
    layout.setTextOption(option);

    QFontMetrics metrics(font);
    int leading = metrics.leading();

    qreal lineWidth = videoSize.width()*.9;
    qreal margin = videoSize.width()*.05;
    qreal height = 0;
    qreal textWidth = 0;
    layout.beginLayout();
    while (1) {
        QTextLine line = layout.createLine();
        if (!line.isValid())
            break;

        line.setLineWidth(lineWidth);
        height += leading;
        line.setPosition(QPointF(margin, height));
        height += line.height();
        textWidth = qMax(textWidth, line.naturalTextWidth());
    }
    layout.endLayout();

    // put subtitles vertically in lower part of the video but not stuck to the bottom
    int bottomMargin = videoSize.height() / 20;
    qreal y = videoSize.height() - bottomMargin - height;
    layout.setPosition(QPointF(0, y));
    textWidth += fontSize/4.;

    bounds = QRectF((videoSize.width() - textWidth)/2., y, textWidth, height);
    return true;
}

void SubtitleLayout::draw(QPainter *painter, const QPointF &translate) const
{
    painter->save();
    painter->translate(translate);
    painter->setCompositionMode(QPainter::CompositionMode_SourceOver);

    QColor bgColor = Qt::black;
    bgColor.setAlpha(128);
    painter->setBrush(bgColor);
    painter->setPen(Qt::NoPen);
    painter->drawRect(bounds);

    QTextLayout::FormatRange range;
    range.start = 0;
    range.length = layout.text().size();
    range.format.setForeground(Qt::white);
    layout.draw(painter, {}, { range });
    painter->restore();
}

QImage SubtitleLayout::toImage() const
{
    auto size = bounds.size().toSize();
    if (size.isEmpty())
        return QImage();
    QImage img(size, QImage::Format_RGBA8888_Premultiplied);
    QColor bgColor = Qt::black;
    bgColor.setAlpha(128);
    img.fill(bgColor);

    QPainter painter(&img);
    painter.translate(-bounds.topLeft());
    QTextLayout::FormatRange range;
    range.start = 0;
    range.length = layout.text().size();
    range.format.setForeground(Qt::white);
    layout.draw(&painter, {}, { range });
    return img;
}

}

QT_END_NAMESPACE
