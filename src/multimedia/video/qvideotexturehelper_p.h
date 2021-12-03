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
#include <private/qrhi_p.h>

#include <QtGui/qtextlayout.h>

QT_BEGIN_NAMESPACE

class QVideoFrame;
class QTextLayout;

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

Q_MULTIMEDIA_EXPORT QString vertexShaderFileName(QVideoFrameFormat::PixelFormat format);
Q_MULTIMEDIA_EXPORT QString fragmentShaderFileName(QVideoFrameFormat::PixelFormat format);
Q_MULTIMEDIA_EXPORT void updateUniformData(QByteArray *dst, const QVideoFrameFormat &format, const QVideoFrame &frame, const QMatrix4x4 &transform, float opacity);
Q_MULTIMEDIA_EXPORT int updateRhiTextures(QVideoFrame frame, QRhi *rhi,
                                           QRhiResourceUpdateBatch *resourceUpdates, QRhiTexture **textures);

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
