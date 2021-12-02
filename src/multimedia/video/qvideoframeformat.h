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
#include <QtCore/qmetatype.h>
#include <QtCore/qshareddata.h>
#include <QtCore/qsize.h>
#include <QtGui/qimage.h>

QT_BEGIN_NAMESPACE


class QDebug;

class QVideoFrameFormatPrivate;
class QVideoFrame;
class QMatrix4x4;

QT_DECLARE_QESDP_SPECIALIZATION_DTOR_WITH_EXPORT(QVideoFrameFormatPrivate, Q_MULTIMEDIA_EXPORT)

class Q_MULTIMEDIA_EXPORT QVideoFrameFormat
{
public:
    enum PixelFormat
    {
        Format_Invalid,
        Format_ARGB8888,
        Format_ARGB8888_Premultiplied,
        Format_XRGB8888,
        Format_BGRA8888,
        Format_BGRA8888_Premultiplied,
        Format_BGRX8888,
        Format_ABGR8888,
        Format_XBGR8888,
        Format_RGBA8888,
        Format_RGBX8888,

        Format_AYUV,
        Format_AYUV_Premultiplied,
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

        Format_P010,
        Format_P016,

        Format_SamplerExternalOES,
        Format_Jpeg,
        Format_SamplerRect,
    };
#ifndef Q_QDOC
    static constexpr int NPixelFormats = Format_SamplerRect + 1;
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
        YCbCr_BT2020
    };

    QVideoFrameFormat();
    QVideoFrameFormat(const QSize &size, PixelFormat pixelFormat);
    QVideoFrameFormat(const QVideoFrameFormat &format);
    ~QVideoFrameFormat();

    QVideoFrameFormat(QVideoFrameFormat &&other) noexcept = default;
    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_PURE_SWAP(QVideoFrameFormat);
    void swap(QVideoFrameFormat &other) noexcept
    { qSwap(d, other.d); }

    void detach();

    QVideoFrameFormat &operator=(const QVideoFrameFormat &format);

    bool operator==(const QVideoFrameFormat &format) const;
    bool operator!=(const QVideoFrameFormat &format) const;

    bool isValid() const;

    QVideoFrameFormat::PixelFormat pixelFormat() const;

    QSize frameSize() const;
    void setFrameSize(const QSize &size);
    void setFrameSize(int width, int height);

    int frameWidth() const;
    int frameHeight() const;

    int planeCount() const;

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

    QString vertexShaderFileName() const;
    QString fragmentShaderFileName() const;
    void updateUniformData(QByteArray *dst, const QVideoFrame &frame, const QMatrix4x4 &transform, float opacity) const;

    static PixelFormat pixelFormatFromImageFormat(QImage::Format format);
    static QImage::Format imageFormatFromPixelFormat(PixelFormat format);

    static QString pixelFormatToString(QVideoFrameFormat::PixelFormat pixelFormat);

private:
    QExplicitlySharedDataPointer<QVideoFrameFormatPrivate> d;
};

Q_DECLARE_SHARED(QVideoFrameFormat)

#ifndef QT_NO_DEBUG_STREAM
Q_MULTIMEDIA_EXPORT QDebug operator<<(QDebug, const QVideoFrameFormat &);
Q_MULTIMEDIA_EXPORT QDebug operator<<(QDebug, QVideoFrameFormat::Direction);
Q_MULTIMEDIA_EXPORT QDebug operator<<(QDebug, QVideoFrameFormat::YCbCrColorSpace);
Q_MULTIMEDIA_EXPORT QDebug operator<<(QDebug, QVideoFrameFormat::PixelFormat);
#endif

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QVideoFrameFormat)

#endif

