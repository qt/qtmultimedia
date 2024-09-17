// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qabstractvideobuffer.h"

#include "qvideotexturehelper_p.h"
#include "qvideoframeconverter_p.h"
#include "qvideoframe_p.h"
#include "private/qmultimediautils_p.h"

#include <qpainter.h>
#include <qloggingcategory.h>

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
      [](int stride, int height) { return stride * height * 2; },
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
    { 1, 4,
      [](int stride, int height) { return stride*height; },
     { QRhiTexture::RGBA8, QRhiTexture::UnknownFormat, QRhiTexture::UnknownFormat },
     { { 1, 1 }, { 1, 1 }, { 1, 1 } }
    },
    // Format_SamplerRect
    {
        1, 0,
        [](int, int) { return 0; },
        { QRhiTexture::BGRA8, QRhiTexture::UnknownFormat, QRhiTexture::UnknownFormat },
        { { 1, 1 }, { 1, 1 }, { 1, 1 } }
    },
    // Format_YUV420P10
    { 3, 2,
        [](int stride, int height) { return stride * ((height * 3 / 2 + 1) & ~1); },
        { QRhiTexture::R16, QRhiTexture::R16, QRhiTexture::R16 },
        { { 1, 1 }, { 2, 2 }, { 2, 2 } }
    },
};

const TextureDescription *textureDescription(QVideoFrameFormat::PixelFormat format)
{
    return descriptions + format;
}

QString vertexShaderFileName(const QVideoFrameFormat &format)
{
    auto fmt = format.pixelFormat();
    Q_UNUSED(fmt);

#if 1//def Q_OS_ANDROID
    if (fmt == QVideoFrameFormat::Format_SamplerExternalOES)
        return QStringLiteral(":/qt-project.org/multimedia/shaders/externalsampler.vert.qsb");
#endif
#if 1//def Q_OS_MACOS
    if (fmt == QVideoFrameFormat::Format_SamplerRect)
        return QStringLiteral(":/qt-project.org/multimedia/shaders/rectsampler.vert.qsb");
#endif

    return QStringLiteral(":/qt-project.org/multimedia/shaders/vertex.vert.qsb");
}

QString fragmentShaderFileName(const QVideoFrameFormat &format, QRhiSwapChain::Format surfaceFormat)
{
    const char *shader = nullptr;
    switch (format.pixelFormat()) {
    case QVideoFrameFormat::Format_Y8:
    case QVideoFrameFormat::Format_Y16:
        shader = "y";
        break;
    case QVideoFrameFormat::Format_AYUV:
    case QVideoFrameFormat::Format_AYUV_Premultiplied:
        shader = "ayuv";
        break;
    case QVideoFrameFormat::Format_ARGB8888:
    case QVideoFrameFormat::Format_ARGB8888_Premultiplied:
    case QVideoFrameFormat::Format_XRGB8888:
        shader = "argb";
        break;
    case QVideoFrameFormat::Format_ABGR8888:
    case QVideoFrameFormat::Format_XBGR8888:
        shader = "abgr";
        break;
    case QVideoFrameFormat::Format_Jpeg: // Jpeg is decoded transparently into an ARGB texture
        shader = "bgra";
        break;
    case QVideoFrameFormat::Format_RGBA8888:
    case QVideoFrameFormat::Format_RGBX8888:
    case QVideoFrameFormat::Format_BGRA8888:
    case QVideoFrameFormat::Format_BGRA8888_Premultiplied:
    case QVideoFrameFormat::Format_BGRX8888:
        shader = "rgba";
        break;
    case QVideoFrameFormat::Format_YUV420P:
    case QVideoFrameFormat::Format_YUV422P:
    case QVideoFrameFormat::Format_IMC3:
        shader = "yuv_triplanar";
        break;
    case QVideoFrameFormat::Format_YUV420P10:
        shader = "yuv_triplanar_p10";
        break;
    case QVideoFrameFormat::Format_YV12:
    case QVideoFrameFormat::Format_IMC1:
        shader = "yvu_triplanar";
        break;
    case QVideoFrameFormat::Format_IMC2:
        shader = "imc2";
        break;
    case QVideoFrameFormat::Format_IMC4:
        shader = "imc4";
        break;
    case QVideoFrameFormat::Format_UYVY:
        shader = "uyvy";
        break;
    case QVideoFrameFormat::Format_YUYV:
        shader = "yuyv";
        break;
    case QVideoFrameFormat::Format_P010:
    case QVideoFrameFormat::Format_P016:
        // P010/P016 have the same layout as NV12, just 16 instead of 8 bits per pixel
        if (format.colorTransfer() == QVideoFrameFormat::ColorTransfer_ST2084) {
            shader = "nv12_bt2020_pq";
            break;
        }
        if (format.colorTransfer() == QVideoFrameFormat::ColorTransfer_STD_B67) {
            shader = "nv12_bt2020_hlg";
            break;
        }
        // Fall through, should be bt709
        Q_FALLTHROUGH();
    case QVideoFrameFormat::Format_NV12:
        shader = "nv12";
        break;
    case QVideoFrameFormat::Format_NV21:
        shader = "nv21";
        break;
    case QVideoFrameFormat::Format_SamplerExternalOES:
#if 1//def Q_OS_ANDROID
        shader = "externalsampler";
        break;
#endif
    case QVideoFrameFormat::Format_SamplerRect:
#if 1//def Q_OS_MACOS
        shader = "rectsampler_bgra";
        break;
#endif
        // fallthrough
    case QVideoFrameFormat::Format_Invalid:
    default:
        break;
    }
    if (!shader)
        return QString();
    QString shaderFile = QStringLiteral(":/qt-project.org/multimedia/shaders/") + QString::fromLatin1(shader);
    if (surfaceFormat == QRhiSwapChain::HDRExtendedSrgbLinear)
        shaderFile += QLatin1String("_linear");
    shaderFile += QStringLiteral(".frag.qsb");
    return shaderFile;
}

// Matrices are calculated from
// https://www.itu.int/dms_pubrec/itu-r/rec/bt/R-REC-BT.601-7-201103-I!!PDF-E.pdf
// https://www.itu.int/dms_pubrec/itu-r/rec/bt/R-REC-BT.709-6-201506-I!!PDF-E.pdf
// https://www.itu.int/dms_pubrec/itu-r/rec/bt/R-REC-BT.2020-2-201510-I!!PDF-E.pdf
//
// For BT2020, we also need to convert the Rec2020 RGB colorspace to sRGB see
// shaders/colorconvert.glsl for details.
//
// Doing the math gives the following (Y, U & V normalized to [0..1] range):
//
// Y = a*R + b*G + c*B
// R = Y           + e*V
// G = Y - c*d/b*U - a*e/b*V
// B = Y + d*U

// BT2020:
// a = .2627, b = 0.6780, c = 0.0593
// d = 1.8814
// e = 1.4746
//
// BT709:
// a = 0.2126, b = 0.7152, c = 0.0722
// d = 1.8556
// e = 1.5748
//
// BT601:
// a = 0.299, b = 0.578, c = 0.114
// d = 1.42
// e = 1.772
//

// clang-format off
static QMatrix4x4 colorMatrix(const QVideoFrameFormat &format)
{
    auto colorSpace = format.colorSpace();
    if (colorSpace == QVideoFrameFormat::ColorSpace_Undefined) {
        if (format.frameHeight() > 576)
            // HD video, assume BT709
            colorSpace = QVideoFrameFormat::ColorSpace_BT709;
        else
            // SD video, assume BT601
            colorSpace = QVideoFrameFormat::ColorSpace_BT601;
    }
    switch (colorSpace) {
    case QVideoFrameFormat::ColorSpace_AdobeRgb:
        return {
            1.0f,  0.000f,  1.402f, -0.701f,
            1.0f, -0.344f, -0.714f,  0.529f,
            1.0f,  1.772f,  0.000f, -0.886f,
            0.0f,  0.000f,  0.000f,  1.000f
        };
    default:
    case QVideoFrameFormat::ColorSpace_BT709:
        if (format.colorRange() == QVideoFrameFormat::ColorRange_Full)
            return {
                1.0f,  0.0f,       1.5748f,   -0.790488f,
                1.0f, -0.187324f, -0.468124f,  0.329010f,
                1.0f,  1.855600f,  0.0f,      -0.931439f,
                0.0f,  0.0f,       0.0f,       1.0f
            };
        return {
            1.1644f,  0.0000f,  1.7927f, -0.9729f,
            1.1644f, -0.2132f, -0.5329f,  0.3015f,
            1.1644f,  2.1124f,  0.0000f, -1.1334f,
            0.0000f,  0.0000f,  0.0000f,  1.0000f
        };
    case QVideoFrameFormat::ColorSpace_BT2020:
        if (format.colorRange() == QVideoFrameFormat::ColorRange_Full)
            return {
                1.f,  0.0000f,  1.4746f, -0.7402f,
                1.f, -0.1646f, -0.5714f,  0.3694f,
                1.f,  1.8814f,  0.000f,  -0.9445f,
                0.0f, 0.0000f,  0.000f,   1.0000f
            };
        return {
            1.1644f,  0.000f,   1.6787f, -0.9157f,
            1.1644f, -0.1874f, -0.6504f,  0.3475f,
            1.1644f,  2.1418f,  0.0000f, -1.1483f,
            0.0000f,  0.0000f,  0.0000f,  1.0000f
        };
    case QVideoFrameFormat::ColorSpace_BT601:
        // Corresponds to the primaries used by NTSC BT601. For PAL BT601, we use the BT709 conversion
        // as those are very close.
        if (format.colorRange() == QVideoFrameFormat::ColorRange_Full)
            return {
                1.f,  0.000f,   1.772f,   -0.886f,
                1.f, -0.1646f, -0.57135f,  0.36795f,
                1.f,  1.42f,    0.000f,   -0.71f,
                0.0f, 0.000f,   0.000f,    1.0000f
            };
        return {
            1.164f,  0.000f,  1.596f, -0.8708f,
            1.164f, -0.392f, -0.813f,  0.5296f,
            1.164f,  2.017f,  0.000f, -1.0810f,
            0.000f,  0.000f,  0.000f,  1.0000f
        };
    }
}
// clang-format on

// PQ transfer function, see also https://en.wikipedia.org/wiki/Perceptual_quantizer
// or https://ieeexplore.ieee.org/document/7291452
static float convertPQFromLinear(float sig)
{
    const float m1 = 1305.f/8192.f;
    const float m2 = 2523.f/32.f;
    const float c1 = 107.f/128.f;
    const float c2 = 2413.f/128.f;
    const float c3 = 2392.f/128.f;

    const float SDR_LEVEL = 100.f;
    sig *= SDR_LEVEL/10000.f;
    float psig = powf(sig, m1);
    float num = c1 + c2*psig;
    float den = 1 + c3*psig;
    return powf(num/den, m2);
}

float convertHLGFromLinear(float sig)
{
    const float a = 0.17883277f;
    const float b = 0.28466892f; // = 1 - 4a
    const float c = 0.55991073f; // = 0.5 - a ln(4a)

    if (sig < 1.f/12.f)
        return sqrtf(3.f*sig);
    return a*logf(12.f*sig - b) + c;
}

static float convertSDRFromLinear(float sig)
{
    return sig;
}

void updateUniformData(QByteArray *dst, const QVideoFrameFormat &format, const QVideoFrame &frame, const QMatrix4x4 &transform, float opacity, float maxNits)
{
#ifndef Q_OS_ANDROID
    Q_UNUSED(frame);
#endif

    QMatrix4x4 cmat;
    switch (format.pixelFormat()) {
    case QVideoFrameFormat::Format_Invalid:
        return;

    case QVideoFrameFormat::Format_Jpeg:
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
    case QVideoFrameFormat::Format_YUV420P10:
    case QVideoFrameFormat::Format_YUV422P:
    case QVideoFrameFormat::Format_YV12:
    case QVideoFrameFormat::Format_UYVY:
    case QVideoFrameFormat::Format_YUYV:
    case QVideoFrameFormat::Format_NV12:
    case QVideoFrameFormat::Format_NV21:
    case QVideoFrameFormat::Format_P010:
    case QVideoFrameFormat::Format_P016:
        cmat = colorMatrix(format);
        break;
    case QVideoFrameFormat::Format_SamplerExternalOES:
        // get Android specific transform for the externalsampler texture
        if (auto hwBuffer = QVideoFramePrivate::hwBuffer(frame))
            cmat = hwBuffer->externalTextureMatrix();
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

    // HDR with a PQ or HLG transfer function uses a BT2390 based tone mapping to cut off the HDR peaks
    // This requires that we pass the max luminance the tonemapper should clip to over to the fragment
    // shader. To reduce computations there, it's precomputed in PQ values here.
    auto fromLinear = convertSDRFromLinear;
    switch (format.colorTransfer()) {
    case QVideoFrameFormat::ColorTransfer_ST2084:
        fromLinear = convertPQFromLinear;
        break;
    case QVideoFrameFormat::ColorTransfer_STD_B67:
        fromLinear = convertHLGFromLinear;
        break;
    default:
        break;
    }

    if (dst->size() < qsizetype(sizeof(UniformData)))
        dst->resize(sizeof(UniformData));

    auto ud = reinterpret_cast<UniformData*>(dst->data());
    memcpy(ud->transformMatrix, transform.constData(), sizeof(ud->transformMatrix));
    memcpy(ud->colorMatrix, cmat.constData(), sizeof(ud->transformMatrix));
    ud->opacity = opacity;
    ud->width = float(format.frameWidth());
    ud->masteringWhite = fromLinear(float(format.maxLuminance())/100.f);
    ud->maxLum = fromLinear(float(maxNits)/100.f);
}

enum class UpdateTextureWithMapResult : uint8_t {
    Failed,
    UpdatedWithDataCopy,
    UpdatedWithDataReference
};

static UpdateTextureWithMapResult updateTextureWithMap(const QVideoFrame &frame, QRhi *rhi,
                                                       QRhiResourceUpdateBatch *rub, int plane,
                                                       std::unique_ptr<QRhiTexture> &tex)
{
    Q_ASSERT(frame.isMapped());

    QVideoFrameFormat fmt = frame.surfaceFormat();
    QVideoFrameFormat::PixelFormat pixelFormat = fmt.pixelFormat();
    QSize size = fmt.frameSize();

    const TextureDescription &texDesc = descriptions[pixelFormat];
    QSize planeSize(size.width()/texDesc.sizeScale[plane].x, size.height()/texDesc.sizeScale[plane].y);

    bool needsRebuild = !tex || tex->pixelSize() != planeSize || tex->format() != texDesc.textureFormat[plane];
    if (!tex) {
        tex.reset(rhi->newTexture(texDesc.textureFormat[plane], planeSize, 1, {}));
        if (!tex) {
            qWarning("Failed to create new texture (size %dx%d)", planeSize.width(), planeSize.height());
            return UpdateTextureWithMapResult::Failed;
        }
    }

    if (needsRebuild) {
        tex->setFormat(texDesc.textureFormat[plane]);
        tex->setPixelSize(planeSize);
        if (!tex->create()) {
            qWarning("Failed to create texture (size %dx%d)", planeSize.width(), planeSize.height());
            return UpdateTextureWithMapResult::Failed;
        }
    }

    auto result = UpdateTextureWithMapResult::UpdatedWithDataCopy;

    QRhiTextureSubresourceUploadDescription subresDesc;

    if (pixelFormat == QVideoFrameFormat::Format_Jpeg) {
        Q_ASSERT(plane == 0);

        QImage image;

        // calling QVideoFrame::toImage is not accurate. To be fixed.
        // frame transformation will be considered later
        const QVideoFrameFormat surfaceFormat = frame.surfaceFormat();

        const bool hasSurfaceTransform = surfaceFormat.isMirrored()
                || surfaceFormat.scanLineDirection() == QVideoFrameFormat::BottomToTop
                || surfaceFormat.rotation() != QtVideo::Rotation::None;

        if (hasSurfaceTransform)
            image = qImageFromVideoFrame(frame, NormalizedFrameTransformation{});
        else
            image = frame.toImage(); // use the frame cache, no surface transforms applied

        image.convertTo(QImage::Format_ARGB32);
        subresDesc.setImage(image);

    } else {
        // Note, QByteArray::fromRawData creare QByteArray as a view without data copying
        subresDesc.setData(QByteArray::fromRawData(
                reinterpret_cast<const char *>(frame.bits(plane)), frame.mappedBytes(plane)));
        subresDesc.setDataStride(frame.bytesPerLine(plane));
        result = UpdateTextureWithMapResult::UpdatedWithDataReference;
    }

    QRhiTextureUploadEntry entry(0, 0, subresDesc);
    QRhiTextureUploadDescription desc({ entry });
    rub->uploadTexture(tex.get(), desc);

    return result;
}

static std::unique_ptr<QRhiTexture> createTextureFromHandle(const QVideoFrame &frame, QRhi *rhi, int plane)
{
    QHwVideoBuffer *hwBuffer = QVideoFramePrivate::hwBuffer(frame);
    Q_ASSERT(hwBuffer);

    QVideoFrameFormat fmt = frame.surfaceFormat();
    QVideoFrameFormat::PixelFormat pixelFormat = fmt.pixelFormat();
    QSize size = fmt.frameSize();

    const TextureDescription &texDesc = descriptions[pixelFormat];
    QSize planeSize(size.width()/texDesc.sizeScale[plane].x, size.height()/texDesc.sizeScale[plane].y);

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

    if (quint64 handle = hwBuffer->textureHandle(rhi, plane); handle) {
        std::unique_ptr<QRhiTexture> tex(rhi->newTexture(texDesc.textureFormat[plane], planeSize, 1, textureFlags));
        if (tex->createFrom({handle, 0}))
            return tex;

        qWarning("Failed to initialize QRhiTexture wrapper for native texture object %llu",handle);
    }
    return {};
}

class QVideoFrameTexturesArray : public QVideoFrameTextures
{
public:
    using TextureArray = std::array<std::unique_ptr<QRhiTexture>, TextureDescription::maxPlanes>;
    QVideoFrameTexturesArray(TextureArray &&textures, QVideoFrame mappedFrame = {})
        : m_textures(std::move(textures)), m_mappedFrame(std::move(mappedFrame))
    {
        Q_ASSERT(!m_mappedFrame.isValid() || m_mappedFrame.isReadable());
    }

    // We keep the source frame mapped during the target texture lifetime.
    // Alternatively, we may use setting a custom image to QRhiTextureSubresourceUploadDescription,
    // unsig videoFramePlaneAsImage, however, the OpenGL rendering pipeline in QRhi
    // may keep QImage, and consequently the mapped QVideoFrame,
    // even after the target texture is deleted: QTBUG-123174.
    ~QVideoFrameTexturesArray() { m_mappedFrame.unmap(); }

    QRhiTexture *texture(uint plane) const override
    {
        return plane < std::size(m_textures) ? m_textures[plane].get() : nullptr;
    }

    TextureArray takeTextures() { return std::move(m_textures); }

private:
    TextureArray m_textures;
    QVideoFrame m_mappedFrame;
};

static std::unique_ptr<QVideoFrameTextures> createTexturesFromHandles(const QVideoFrame &frame, QRhi *rhi)
{
    const TextureDescription &texDesc = descriptions[frame.surfaceFormat().pixelFormat()];
    bool ok = true;
    QVideoFrameTexturesArray::TextureArray textures;
    for (quint8 plane = 0; plane < texDesc.nplanes; ++plane) {
        textures[plane] = QVideoTextureHelper::createTextureFromHandle(frame, rhi, plane);
        ok &= bool(textures[plane]);
    }
    if (ok)
        return std::make_unique<QVideoFrameTexturesArray>(std::move(textures));
    else
        return {};
}

static std::unique_ptr<QVideoFrameTextures> createTexturesFromMemory(QVideoFrame frame, QRhi *rhi, QRhiResourceUpdateBatch *rub, QVideoFrameTextures *old)
{
    const TextureDescription &texDesc = descriptions[frame.surfaceFormat().pixelFormat()];
    QVideoFrameTexturesArray::TextureArray textures;
    auto oldArray = dynamic_cast<QVideoFrameTexturesArray *>(old);
    if (oldArray)
        textures = oldArray->takeTextures();

    if (!frame.map(QVideoFrame::ReadOnly)) {
        qWarning() << "Cannot map a video frame in ReadOnly mode!";
        return {};
    }

    auto unmapFrameGuard = qScopeGuard([&frame] { frame.unmap(); });

    bool shouldKeepMapping = false;
    for (quint8 plane = 0; plane < texDesc.nplanes; ++plane) {
        const auto result = updateTextureWithMap(frame, rhi, rub, plane, textures[plane]);
        if (result == UpdateTextureWithMapResult::Failed)
            return {};

        if (result == UpdateTextureWithMapResult::UpdatedWithDataReference)
            shouldKeepMapping = true;
    }

    // as QVideoFrame::unmap does nothing with null frames, we just move the frame to the result
    return std::make_unique<QVideoFrameTexturesArray>(
            std::move(textures), shouldKeepMapping ? std::move(frame) : QVideoFrame());
}

std::unique_ptr<QVideoFrameTextures> createTextures(QVideoFrame &frame, QRhi *rhi, QRhiResourceUpdateBatch *rub, std::unique_ptr<QVideoFrameTextures> &&oldTextures)
{
    if (!frame.isValid())
        return {};

    if (QHwVideoBuffer *hwBuffer = QVideoFramePrivate::hwBuffer(frame)) {
        if (auto textures = hwBuffer->mapTextures(rhi))
            return textures;

        if (auto textures = createTexturesFromHandles(frame, rhi))
            return textures;
    }

    return createTexturesFromMemory(frame, rhi, rub, oldTextures.get());
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
