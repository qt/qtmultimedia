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

QT_BEGIN_NAMESPACE

namespace QVideoTextureHelper
{

static const TextureDescription descriptions[QVideoFrameFormat::NPixelFormats] = {
    //  Format_Invalid
    { 0, 0,
      [](int, int) { return 0; },
     { QRhiTexture::UnknownFormat, QRhiTexture::UnknownFormat, QRhiTexture::UnknownFormat},
     { { 1, 1 }, { 1, 1 }, { 1, 1 } }
    },
    // Format_ARGB32
    { 1, 4,
      [](int stride, int height) { return stride*height; },
     { QRhiTexture::BGRA8, QRhiTexture::UnknownFormat, QRhiTexture::UnknownFormat },
     { { 1, 1 }, { 1, 1 }, { 1, 1 } }
    },
     // Format_ARGB32_Premultiplied
    { 1, 4,
      [](int stride, int height) { return stride*height; },
     { QRhiTexture::BGRA8, QRhiTexture::UnknownFormat, QRhiTexture::UnknownFormat },
     { { 1, 1 }, { 1, 1 }, { 1, 1 } }
    },
    // Format_RGB32
    { 1, 4,
      [](int stride, int height) { return stride*height; },
     { QRhiTexture::BGRA8, QRhiTexture::UnknownFormat, QRhiTexture::UnknownFormat },
     { { 1, 1 }, { 1, 1 }, { 1, 1 } }
    },
    // Format_BGRA32
    { 1, 4,
      [](int stride, int height) { return stride*height; },
     { QRhiTexture::BGRA8, QRhiTexture::UnknownFormat, QRhiTexture::UnknownFormat },
     { { 1, 1 }, { 1, 1 }, { 1, 1 } }
    },
    // Format_BGRA32_Premultiplied
    { 1, 4,
      [](int stride, int height) { return stride*height; },
     { QRhiTexture::BGRA8, QRhiTexture::UnknownFormat, QRhiTexture::UnknownFormat },
     { { 1, 1 }, { 1, 1 }, { 1, 1 } }
    },
    // Format_ABGR32
    { 1, 4,
      [](int stride, int height) { return stride*height; },
     { QRhiTexture::BGRA8, QRhiTexture::UnknownFormat, QRhiTexture::UnknownFormat },
     { { 1, 1 }, { 1, 1 }, { 1, 1 } }
    },
    // Format_BGR32
    { 1, 4,
      [](int stride, int height) { return stride*height; },
     { QRhiTexture::BGRA8, QRhiTexture::UnknownFormat, QRhiTexture::UnknownFormat },
     { { 1, 1 }, { 1, 1 }, { 1, 1 } }
    },

    // Format_AYUV444
    { 1, 4,
      [](int stride, int height) { return stride*height; },
     { QRhiTexture::BGRA8, QRhiTexture::UnknownFormat, QRhiTexture::UnknownFormat },
     { { 1, 1 }, { 1, 1 }, { 1, 1 } }
    },
    // Format_AYUV444_Premultiplied
    { 1, 4,
      [](int stride, int height) { return stride*height; },
     { QRhiTexture::BGRA8, QRhiTexture::UnknownFormat, QRhiTexture::UnknownFormat },
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
     { QRhiTexture::BGRA8, QRhiTexture::UnknownFormat, QRhiTexture::UnknownFormat },
     { { 2, 1 }, { 1, 1 }, { 1, 1 } }
    },
    // Format_YUYV
    { 1, 2,
      [](int stride, int height) { return stride*height; },
     { QRhiTexture::BGRA8, QRhiTexture::UnknownFormat, QRhiTexture::UnknownFormat },
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
#if QT_VERSION < QT_VERSION_CHECK(6, 2, 0)
    // Rendering of those formats pre 6.2 will be wrong, as RHI doesn't yet support the correct texture formats
    // Format_P010
    { 2, 2,
     [](int stride, int height) { return stride * ((height * 3 / 2 + 1) & ~1); },
     { QRhiTexture::RG8, QRhiTexture::RGBA8, QRhiTexture::UnknownFormat },
     { { 1, 1 }, { 2, 2 }, { 1, 1 } }
    },
    // Format_P016
    { 2, 2,
     [](int stride, int height) { return stride * ((height * 3 / 2 + 1) & ~1); },
     { QRhiTexture::RG8, QRhiTexture::RGBA8, QRhiTexture::UnknownFormat },
     { { 1, 1 }, { 2, 2 }, { 1, 1 } }
    },
#else
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
#endif
    // Format_Jpeg
    { 1, 0,
      [](int, int) { return 0; },
     { QRhiTexture::UnknownFormat, QRhiTexture::UnknownFormat, QRhiTexture::UnknownFormat },
     { { 1, 1 }, { 1, 1 }, { 1, 1 } }
    }
};


const TextureDescription *textureDescription(QVideoFrameFormat::PixelFormat format)
{
    return descriptions + format;
}

QString vertexShaderFileName(QVideoFrameFormat::PixelFormat /*format*/)
{
    return QStringLiteral(":/qt-project.org/multimedia/shaders/vertex.vert.qsb");
}

QString fragmentShaderFileName(QVideoFrameFormat::PixelFormat format)
{
    switch (format) {
    case QVideoFrameFormat::Format_Y8:
    case QVideoFrameFormat::Format_Y16:
        return QStringLiteral(":/qt-project.org/multimedia/shaders/y.frag.qsb");
    case QVideoFrameFormat::Format_AYUV444:
    case QVideoFrameFormat::Format_AYUV444_Premultiplied:
        return QStringLiteral(":/qt-project.org/multimedia/shaders/ayuv.frag.qsb");
    case QVideoFrameFormat::Format_ARGB32:
    case QVideoFrameFormat::Format_ARGB32_Premultiplied:
    case QVideoFrameFormat::Format_RGB32:
        return QStringLiteral(":/qt-project.org/multimedia/shaders/argb.frag.qsb");
    case QVideoFrameFormat::Format_BGRA32:
    case QVideoFrameFormat::Format_BGRA32_Premultiplied:
        return QStringLiteral(":/qt-project.org/multimedia/shaders/bgra.frag.qsb");
    case QVideoFrameFormat::Format_ABGR32:
    case QVideoFrameFormat::Format_BGR32:
        return QStringLiteral(":/qt-project.org/multimedia/shaders/abgr.frag.qsb");
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
            1.164f,  0.000f,  1.793f, -0.5727f,
            1.164f, -0.534f, -0.213f,  0.3007f,
            1.164f,  2.115f,  0.000f, -1.1302f,
            0.0f,    0.000f,  0.000f,  1.0000f);
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

QByteArray uniformData(const QVideoFrameFormat &format, const QMatrix4x4 &transform, float opacity)
{
    QMatrix4x4 cmat;
    switch (format.pixelFormat()) {
    case QVideoFrameFormat::Format_Invalid:
    case QVideoFrameFormat::Format_Jpeg:
        return QByteArray();

    case QVideoFrameFormat::Format_ARGB32:
    case QVideoFrameFormat::Format_ARGB32_Premultiplied:
    case QVideoFrameFormat::Format_RGB32:
    case QVideoFrameFormat::Format_BGRA32:
    case QVideoFrameFormat::Format_BGRA32_Premultiplied:
    case QVideoFrameFormat::Format_ABGR32:
    case QVideoFrameFormat::Format_BGR32:

    case QVideoFrameFormat::Format_Y8:
    case QVideoFrameFormat::Format_Y16:
        break;
    case QVideoFrameFormat::Format_IMC1:
    case QVideoFrameFormat::Format_IMC2:
    case QVideoFrameFormat::Format_IMC3:
    case QVideoFrameFormat::Format_IMC4:
    case QVideoFrameFormat::Format_AYUV444:
    case QVideoFrameFormat::Format_AYUV444_Premultiplied:
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
    }
    // { matrix4x4, colorMatrix, opacity, planeWidth[3] }
    QByteArray buf(64*2 + 4 + 4, Qt::Uninitialized);
    char *data = buf.data();
    memcpy(data, transform.constData(), 64);
    memcpy(data + 64, cmat.constData(), 64);
    memcpy(data + 64 + 64, &opacity, 4);
    float width = format.frameWidth();
    memcpy(data + 64 + 64 + 4, &width, 4);
    return buf;
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
        for (int plane = 0; plane < description->nplanes; ++plane) {
            quint64 nativeTexture = frame.textureHandle(plane);
            Q_ASSERT(nativeTexture);
            textures[plane] = rhi->newTexture(description->textureFormat[plane], planeSizes[plane], 1, {});
            textures[plane]->createFrom({nativeTexture, 0});
        }
        return description->nplanes;
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

        QRhiTextureSubresourceUploadDescription subresDesc(frame.bits(plane), frame.mappedBytes(plane));
        subresDesc.setDataStride(frame.bytesPerLine(plane));
        QRhiTextureUploadEntry entry(0, 0, subresDesc);
        QRhiTextureUploadDescription desc({ entry });
        resourceUpdates->uploadTexture(textures[plane], desc);
    }
    return description->nplanes;
}

}

QT_END_NAMESPACE
