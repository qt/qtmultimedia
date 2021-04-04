/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#ifndef QVIDEOSURFACEFORMAT_H
#define QVIDEOSURFACEFORMAT_H

#include <QtMultimedia/qtmultimediaglobal.h>

#include <QtCore/qlist.h>
#include <QtCore/qpair.h>
#include <QtCore/qshareddata.h>
#include <QtCore/qsize.h>
#include <QtGui/qimage.h>

QT_BEGIN_NAMESPACE


class QDebug;

class QVideoSurfaceFormatPrivate;
class QMatrix4x4;

class Q_MULTIMEDIA_EXPORT QVideoSurfaceFormat
{
public:
    enum PixelFormat
    {
        Format_Invalid,
        Format_ARGB32,
        Format_ARGB32_Premultiplied,
        Format_RGB32,
        Format_RGB565,
        Format_RGB555,
        Format_BGRA32,
        Format_BGRA32_Premultiplied,
        Format_ABGR32,
        Format_BGR32,
        Format_BGR565,
        Format_BGR555,

        Format_AYUV444,
        Format_AYUV444_Premultiplied,
        Format_YUV420P,
        Format_YUV422P,
        Format_YV12,
        Format_UYVY,
        Format_YUYV,
        Format_NV12,
        Format_NV21,
        Format_IMC1,
        Format_IMC2,
        Format_IMC3,
        Format_IMC4,
        Format_Y8,
        Format_Y16,

        Format_P010LE,
        Format_P010BE,
        Format_P016LE,
        Format_P016BE,

        Format_Jpeg,
    };
#ifndef Q_QDOC
    static constexpr int NPixelFormats = Format_Jpeg + 1;
#endif

    enum Direction
    {
        TopToBottom,
        BottomToTop
    };

    enum YCbCrColorSpace
    {
        YCbCr_Undefined,
        YCbCr_BT601,
        YCbCr_BT709,
        YCbCr_xvYCC601,
        YCbCr_xvYCC709,
        YCbCr_JPEG,
    };

    QVideoSurfaceFormat();
    QVideoSurfaceFormat(
            const QSize &size,
            QVideoSurfaceFormat::PixelFormat pixelFormat);
    QVideoSurfaceFormat(const QVideoSurfaceFormat &format);
    ~QVideoSurfaceFormat();

    QVideoSurfaceFormat &operator =(const QVideoSurfaceFormat &format);

    bool operator ==(const QVideoSurfaceFormat &format) const;
    bool operator !=(const QVideoSurfaceFormat &format) const;

    bool isValid() const;

    QVideoSurfaceFormat::PixelFormat pixelFormat() const;

    QSize frameSize() const;
    void setFrameSize(const QSize &size);
    void setFrameSize(int width, int height);

    int frameWidth() const;
    int frameHeight() const;

    int nPlanes() const;

    QRect viewport() const;
    void setViewport(const QRect &viewport);

    Direction scanLineDirection() const;
    void setScanLineDirection(Direction direction);

    qreal frameRate() const;
    void setFrameRate(qreal rate);

    YCbCrColorSpace yCbCrColorSpace() const;
    void setYCbCrColorSpace(YCbCrColorSpace colorSpace);

    bool isMirrored() const;
    void setMirrored(bool mirrored);

    QSize sizeHint() const;

    QString vertexShaderFileName() const;
    QString fragmentShaderFileName() const;
    QByteArray uniformData(const QMatrix4x4 &transform, float opacity) const;

    static PixelFormat pixelFormatFromImageFormat(QImage::Format format);
    static QImage::Format imageFormatFromPixelFormat(PixelFormat format);

private:
    QSharedDataPointer<QVideoSurfaceFormatPrivate> d;
};

#ifndef QT_NO_DEBUG_STREAM
Q_MULTIMEDIA_EXPORT QDebug operator<<(QDebug, const QVideoSurfaceFormat &);
Q_MULTIMEDIA_EXPORT QDebug operator<<(QDebug, QVideoSurfaceFormat::Direction);
Q_MULTIMEDIA_EXPORT QDebug operator<<(QDebug, QVideoSurfaceFormat::YCbCrColorSpace);
Q_MULTIMEDIA_EXPORT QDebug operator<<(QDebug, QVideoSurfaceFormat::PixelFormat);
#endif

QT_END_NAMESPACE

#endif

