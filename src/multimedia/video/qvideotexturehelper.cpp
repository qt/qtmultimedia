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

namespace QVideoTextureHelper
{

static const TextureDescription descriptions[QVideoSurfaceFormat::NPixelFormats] = {
    //  Format_Invalid
    { 0,
     { QRhiTexture::UnknownFormat, QRhiTexture::UnknownFormat, QRhiTexture::UnknownFormat},
     { { 1, 1 }, { 1, 1 }, { 1, 1 } }
    },
    // Format_ARGB32
    { 1,
     { QRhiTexture::BGRA8, QRhiTexture::UnknownFormat, QRhiTexture::UnknownFormat },
     { { 1, 1 }, { 1, 1 }, { 1, 1 } }
    },
     // Format_ARGB32_Premultiplied
    { 1,
     { QRhiTexture::BGRA8, QRhiTexture::UnknownFormat, QRhiTexture::UnknownFormat },
     { { 1, 1 }, { 1, 1 }, { 1, 1 } }
    },
    // Format_RGB32
    { 1,
     { QRhiTexture::BGRA8, QRhiTexture::UnknownFormat, QRhiTexture::UnknownFormat },
     { { 1, 1 }, { 1, 1 }, { 1, 1 } }
    },
    // Format_RGB565
    { 1,
     { QRhiTexture::UnknownFormat, QRhiTexture::UnknownFormat, QRhiTexture::UnknownFormat },
     { { 1, 1 }, { 1, 1 }, { 1, 1 } }
    },
     // Format_RGB555
    { 1,
     { QRhiTexture::UnknownFormat, QRhiTexture::UnknownFormat, QRhiTexture::UnknownFormat },
     { { 1, 1 }, { 1, 1 }, { 1, 1 } }
    },
    // Format_BGRA32
    { 1,
     { QRhiTexture::BGRA8, QRhiTexture::UnknownFormat, QRhiTexture::UnknownFormat },
     { { 1, 1 }, { 1, 1 }, { 1, 1 } }
    },
    // Format_BGRA32_Premultiplied
    { 1,
     { QRhiTexture::BGRA8, QRhiTexture::UnknownFormat, QRhiTexture::UnknownFormat },
     { { 1, 1 }, { 1, 1 }, { 1, 1 } }
    },
    // Format_ABGR32
    { 1,
     { QRhiTexture::BGRA8, QRhiTexture::UnknownFormat, QRhiTexture::UnknownFormat },
     { { 1, 1 }, { 1, 1 }, { 1, 1 } }
    },
    // Format_BGR32
    { 1,
     { QRhiTexture::BGRA8, QRhiTexture::UnknownFormat, QRhiTexture::UnknownFormat },
     { { 1, 1 }, { 1, 1 }, { 1, 1 } }
    },
     // Format_BGR565
    { 1,
     { QRhiTexture::UnknownFormat, QRhiTexture::UnknownFormat, QRhiTexture::UnknownFormat },
     { { 1, 1 }, { 1, 1 }, { 1, 1 } }
    },
     // Format_BGR555
    { 1,
     { QRhiTexture::UnknownFormat, QRhiTexture::UnknownFormat, QRhiTexture::UnknownFormat },
     { { 1, 1 }, { 1, 1 }, { 1, 1 } }
    },

    // Format_AYUV444
    { 1,
     { QRhiTexture::BGRA8, QRhiTexture::UnknownFormat, QRhiTexture::UnknownFormat },
     { { 1, 1 }, { 1, 1 }, { 1, 1 } }
    },
    // Format_AYUV444_Premultiplied
    { 1,
     { QRhiTexture::BGRA8, QRhiTexture::UnknownFormat, QRhiTexture::UnknownFormat },
     { { 1, 1 }, { 1, 1 }, { 1, 1 } }
    },
    // Format_YUV420P
    { 3,
     { QRhiTexture::R8, QRhiTexture::R8, QRhiTexture::R8 },
     { { 1, 1 }, { 2, 2 }, { 2, 2 } }
    },
     // Format_YUV422P
    { 3,
     { QRhiTexture::R8, QRhiTexture::R8, QRhiTexture::R8 },
     { { 1, 1 }, { 2, 1 }, { 2, 1 } }
    },
     // Format_YV12
    { 3,
     { QRhiTexture::R8, QRhiTexture::R8, QRhiTexture::R8 },
     { { 1, 1 }, { 2, 2 }, { 2, 2 } }
    },
    // Format_UYVY
    { 1,
     { QRhiTexture::BGRA8, QRhiTexture::UnknownFormat, QRhiTexture::UnknownFormat },
     { { 1, 1 }, { 1, 1 }, { 1, 1 } }
    },
    // Format_YUYV
    { 1,
     { QRhiTexture::BGRA8, QRhiTexture::UnknownFormat, QRhiTexture::UnknownFormat },
     { { 1, 1 }, { 1, 1 }, { 1, 1 } }
    },
    // Format_NV12
    { 2,
     { QRhiTexture::RG8, QRhiTexture::R8, QRhiTexture::UnknownFormat },
     { { 1, 1 }, { 2, 2 }, { 1, 1 } }
    },
    // Format_NV21
    { 2,
     { QRhiTexture::RG8, QRhiTexture::R8, QRhiTexture::UnknownFormat },
     { { 1, 1 }, { 2, 2 }, { 1, 1 } }
    },
    // Format_IMC1
    { 3,
     { QRhiTexture::R8, QRhiTexture::R8, QRhiTexture::R8 },
     { { 1, 1 }, { 1, 1 }, { 1, 1 } }
    },
    // Format_IMC2
    { 2,
     { QRhiTexture::R8, QRhiTexture::R8, QRhiTexture::UnknownFormat },
     { { 1, 1 }, { 1, 1 }, { 1, 1 } }
    },
    // Format_IMC3
    { 3,
     { QRhiTexture::R8, QRhiTexture::R8, QRhiTexture::R8 },
     { { 1, 1 }, { 1, 1 }, { 1, 1 } }
    },
    // Format_IMC4
    { 2,
     { QRhiTexture::R8, QRhiTexture::R8, QRhiTexture::UnknownFormat },
     { { 1, 1 }, { 1, 1 }, { 1, 1 } }
    },
    // Format_Y8
    { 1,
     { QRhiTexture::R8, QRhiTexture::UnknownFormat, QRhiTexture::UnknownFormat },
     { { 1, 1 }, { 1, 1 }, { 1, 1 } }
    },
    // Format_Y16
    { 1,
     { QRhiTexture::R16, QRhiTexture::UnknownFormat, QRhiTexture::UnknownFormat },
     { { 1, 1 }, { 1, 1 }, { 1, 1 } }
    },

    // Format_P010
    { 2,
     { QRhiTexture::R16, QRhiTexture::RG16, QRhiTexture::UnknownFormat },
     { { 1, 1 }, { 2, 2 }, { 1, 1 } }
    },
    // Format_P016
    { 2,
     { QRhiTexture::R16, QRhiTexture::RG16, QRhiTexture::UnknownFormat },
     { { 1, 1 }, { 2, 2 }, { 1, 1 } }
    },

    // Format_Jpeg
    { 1,
     { QRhiTexture::UnknownFormat, QRhiTexture::UnknownFormat, QRhiTexture::UnknownFormat },
     { { 1, 1 }, { 1, 1 }, { 1, 1 } }
    }
};


const TextureDescription *textureDescription(QVideoSurfaceFormat::PixelFormat format)
{
    return descriptions + format;
}

QString vertexShaderFileName(QVideoSurfaceFormat::PixelFormat /*format*/)
{
    return QStringLiteral(":/qt-project.org/multimedia/shaders/vertex.vert.qsb");
}

QString fragmentShaderFileName(QVideoSurfaceFormat::PixelFormat format)
{
    switch (format) {
    case QVideoSurfaceFormat::Format_Invalid:
    case QVideoSurfaceFormat::Format_Jpeg:

    case QVideoSurfaceFormat::Format_RGB565:
    case QVideoSurfaceFormat::Format_RGB555:
    case QVideoSurfaceFormat::Format_BGR565:
    case QVideoSurfaceFormat::Format_BGR555:

    case QVideoSurfaceFormat::Format_IMC1:
    case QVideoSurfaceFormat::Format_IMC2:
    case QVideoSurfaceFormat::Format_IMC3:
    case QVideoSurfaceFormat::Format_IMC4:
        return QString();

    case QVideoSurfaceFormat::Format_Y8:
    case QVideoSurfaceFormat::Format_Y16:
        return QStringLiteral(":/qt-project.org/multimedia/shaders/y.frag.qsb");
    case QVideoSurfaceFormat::Format_AYUV444:
    case QVideoSurfaceFormat::Format_AYUV444_Premultiplied:
        return QStringLiteral(":/qt-project.org/multimedia/shaders/ayuv.frag.qsb");
    case QVideoSurfaceFormat::Format_ARGB32:
    case QVideoSurfaceFormat::Format_ARGB32_Premultiplied:
    case QVideoSurfaceFormat::Format_RGB32:
        return QStringLiteral(":/qt-project.org/multimedia/shaders/argb.frag.qsb");
    case QVideoSurfaceFormat::Format_BGRA32:
    case QVideoSurfaceFormat::Format_BGRA32_Premultiplied:
        return QStringLiteral(":/qt-project.org/multimedia/shaders/bgra.frag.qsb");
    case QVideoSurfaceFormat::Format_ABGR32:
    case QVideoSurfaceFormat::Format_BGR32:
        return QStringLiteral(":/qt-project.org/multimedia/shaders/abgr.frag.qsb");
    case QVideoSurfaceFormat::Format_YUV420P:
    case QVideoSurfaceFormat::Format_YUV422P:
    case QVideoSurfaceFormat::Format_YV12:
        return QStringLiteral(":/qt-project.org/multimedia/shaders/yuv_yv.frag.qsb");
    case QVideoSurfaceFormat::Format_UYVY:
        return QStringLiteral(":/qt-project.org/multimedia/shaders/uyvy.frag.qsb");
    case QVideoSurfaceFormat::Format_YUYV:
        return QStringLiteral(":/qt-project.org/multimedia/shaders/yuyv.frag.qsb");
    case QVideoSurfaceFormat::Format_NV12:
    case QVideoSurfaceFormat::Format_P010:
    case QVideoSurfaceFormat::Format_P016:
        // P010/P016 have the same layout as NV12, just 16 instead of 8 bits per pixel
        return QStringLiteral(":/qt-project.org/multimedia/shaders/nv12.frag.qsb");
    case QVideoSurfaceFormat::Format_NV21:
        return QStringLiteral(":/qt-project.org/multimedia/shaders/nv21.frag.qsb");
    }
}


static QMatrix4x4 colorMatrix(QVideoSurfaceFormat::YCbCrColorSpace colorSpace)
{
    switch (colorSpace) {
    case QVideoSurfaceFormat::YCbCr_JPEG:
        return QMatrix4x4(
            1.0f,  0.000f,  1.402f, -0.701f,
            1.0f, -0.344f, -0.714f,  0.529f,
            1.0f,  1.772f,  0.000f, -0.886f,
            0.0f,  0.000f,  0.000f,  1.0000f);
    case QVideoSurfaceFormat::YCbCr_BT709:
    case QVideoSurfaceFormat::YCbCr_xvYCC709:
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

QByteArray uniformData(const QVideoSurfaceFormat &format, const QMatrix4x4 &transform, float opacity)
{
    QMatrix4x4 cmat;
    switch (format.pixelFormat()) {
    case QVideoSurfaceFormat::Format_Invalid:
    case QVideoSurfaceFormat::Format_Jpeg:

    case QVideoSurfaceFormat::Format_RGB565:
    case QVideoSurfaceFormat::Format_RGB555:
    case QVideoSurfaceFormat::Format_BGR565:
    case QVideoSurfaceFormat::Format_BGR555:

    case QVideoSurfaceFormat::Format_IMC1:
    case QVideoSurfaceFormat::Format_IMC2:
    case QVideoSurfaceFormat::Format_IMC3:
    case QVideoSurfaceFormat::Format_IMC4:
        return QByteArray();
    case QVideoSurfaceFormat::Format_ARGB32:
    case QVideoSurfaceFormat::Format_ARGB32_Premultiplied:
    case QVideoSurfaceFormat::Format_RGB32:
    case QVideoSurfaceFormat::Format_BGRA32:
    case QVideoSurfaceFormat::Format_BGRA32_Premultiplied:
    case QVideoSurfaceFormat::Format_ABGR32:
    case QVideoSurfaceFormat::Format_BGR32:

    case QVideoSurfaceFormat::Format_Y8:
    case QVideoSurfaceFormat::Format_Y16:
        break;
    case QVideoSurfaceFormat::Format_AYUV444:
    case QVideoSurfaceFormat::Format_AYUV444_Premultiplied:
    case QVideoSurfaceFormat::Format_YUV420P:
    case QVideoSurfaceFormat::Format_YUV422P:
    case QVideoSurfaceFormat::Format_YV12:
    case QVideoSurfaceFormat::Format_UYVY:
    case QVideoSurfaceFormat::Format_YUYV:
    case QVideoSurfaceFormat::Format_NV12:
    case QVideoSurfaceFormat::Format_NV21:
    case QVideoSurfaceFormat::Format_P010:
    case QVideoSurfaceFormat::Format_P016:
        cmat = colorMatrix(format.yCbCrColorSpace());
        break;
    }
    // { matrix4x4, colorMatrix, opacity, planeWidth[3] }
    QByteArray buf(64*2 + 4, Qt::Uninitialized);
    char *data = buf.data();
    memcpy(data, transform.constData(), 64);
    memcpy(data + 64, cmat.constData(), 64);
    memcpy(data + 64 + 64, &opacity, 4);
    return buf;
}

int updateRhiTextures(QVideoFrame frame, QRhi *rhi, QRhiResourceUpdateBatch *resourceUpdates, QRhiTexture **textures)
{
    QVideoSurfaceFormat fmt = frame.surfaceFormat();
    QVideoSurfaceFormat::PixelFormat pixelFormat = fmt.pixelFormat();
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

        QRhiTextureSubresourceUploadDescription subresDesc(frame.bits(plane), frame.bytesPerLine(plane)*planeSizes[plane].height());
        subresDesc.setSourceSize(planeSizes[plane]);
        subresDesc.setDestinationTopLeft(QPoint(0, 0));
        QRhiTextureUploadEntry entry(0, 0, subresDesc);
        QRhiTextureUploadDescription desc({ entry });
        resourceUpdates->uploadTexture(textures[plane], desc);
    }
    return description->nplanes;
}

}
