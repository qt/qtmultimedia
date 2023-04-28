// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QVIDEOTEXTUREHELPER_H
#define QVIDEOTEXTUREHELPER_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <qvideoframeformat.h>
#include <rhi/qrhi.h>

#include <QtGui/qtextlayout.h>

QT_BEGIN_NAMESPACE

class QVideoFrame;
class QTextLayout;
class QVideoFrameTextures;

namespace QVideoTextureHelper
{

struct TextureDescription
{
    static constexpr int maxPlanes = 3;
    struct SizeScale {
        int x;
        int y;
    };
    using BytesRequired = int(*)(int stride, int height);

    inline int strideForWidth(int width) const { return (width*strideFactor + 15) & ~15; }
    inline int bytesForSize(QSize s) const { return bytesRequired(strideForWidth(s.width()), s.height()); }
    int widthForPlane(int width, int plane) const
    {
        if (plane > nplanes) return 0;
        return (width + sizeScale[plane].x - 1)/sizeScale[plane].x;
    }
    int heightForPlane(int height, int plane) const
    {
        if (plane > nplanes) return 0;
        return (height + sizeScale[plane].y - 1)/sizeScale[plane].y;
    }

    int nplanes;
    int strideFactor;
    BytesRequired bytesRequired;
    QRhiTexture::Format textureFormat[maxPlanes];
    SizeScale sizeScale[maxPlanes];
};

Q_MULTIMEDIA_EXPORT const TextureDescription *textureDescription(QVideoFrameFormat::PixelFormat format);

Q_MULTIMEDIA_EXPORT QString vertexShaderFileName(const QVideoFrameFormat &format);
Q_MULTIMEDIA_EXPORT QString fragmentShaderFileName(const QVideoFrameFormat &format, QRhiSwapChain::Format surfaceFormat = QRhiSwapChain::SDR);
Q_MULTIMEDIA_EXPORT void updateUniformData(QByteArray *dst, const QVideoFrameFormat &format, const QVideoFrame &frame,
                                           const QMatrix4x4 &transform, float opacity, float maxNits = 100);
Q_MULTIMEDIA_EXPORT std::unique_ptr<QVideoFrameTextures> createTextures(QVideoFrame &frame, QRhi *rhi, QRhiResourceUpdateBatch *rub, std::unique_ptr<QVideoFrameTextures> &&oldTextures);

struct UniformData {
    float transformMatrix[4][4];
    float colorMatrix[4][4];
    float opacity;
    float width;
    float masteringWhite;
    float maxLum;
};

struct Q_MULTIMEDIA_EXPORT SubtitleLayout
{
    QSize videoSize;
    QRectF bounds;
    QTextLayout layout;

    bool update(const QSize &frameSize, QString text);
    void draw(QPainter *painter, const QPointF &translate) const;
    QImage toImage() const;
};

}

QT_END_NAMESPACE

#endif
