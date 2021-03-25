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

#include "qvideosurfaceformat.h"

#include <qdebug.h>
#include <qlist.h>
#include <qmetatype.h>
#include <qpair.h>
#include <qvariant.h>
#include <qmatrix4x4.h>

QT_BEGIN_NAMESPACE

static void initResource() {
    Q_INIT_RESOURCE(qtmultimedia);
}

class QVideoSurfaceFormatPrivate : public QSharedData
{
public:
    QVideoSurfaceFormatPrivate() = default;

    QVideoSurfaceFormatPrivate(
            const QSize &size,
            QVideoSurfaceFormat::PixelFormat format)
        : pixelFormat(format)
        , frameSize(size)
        , viewport(QPoint(0, 0), size)
    {
    }

    bool operator ==(const QVideoSurfaceFormatPrivate &other) const
    {
        if (pixelFormat == other.pixelFormat
            && scanLineDirection == other.scanLineDirection
            && frameSize == other.frameSize
            && viewport == other.viewport
            && frameRatesEqual(frameRate, other.frameRate)
            && ycbcrColorSpace == other.ycbcrColorSpace
            && mirrored == other.mirrored)
            return true;

        return false;
    }

    inline static bool frameRatesEqual(qreal r1, qreal r2)
    {
        return qAbs(r1 - r2) <= 0.00001 * qMin(qAbs(r1), qAbs(r2));
    }

    QVideoSurfaceFormat::PixelFormat pixelFormat = QVideoSurfaceFormat::Format_Invalid;
    QVideoSurfaceFormat::Direction scanLineDirection = QVideoSurfaceFormat::TopToBottom;
    QSize frameSize;
    QVideoSurfaceFormat::YCbCrColorSpace ycbcrColorSpace = QVideoSurfaceFormat::YCbCr_Undefined;
    QRect viewport;
    qreal frameRate = 0.0;
    bool mirrored = false;
};

/*!
    \class QVideoSurfaceFormat
    \brief The QVideoSurfaceFormat class specifies the stream format of a video presentation
    surface.
    \inmodule QtMultimedia

    \ingroup multimedia
    \ingroup multimedia_video

    A video surface presents a stream of video frames.  The surface's format describes the type of
    the frames and determines how they should be presented.

    The core properties of a video stream required to setup a video surface are the pixel format
    given by pixelFormat(), and the frame dimensions given by frameSize().

    If the surface is to present frames using a frame's handle a surface format will also include
    a handle type which is given by the handleType() function.

    The region of a frame that is actually displayed on a video surface is given by the viewport().
    A stream may have a viewport less than the entire region of a frame to allow for videos smaller
    than the nearest optimal size of a video frame.  For example the width of a frame may be
    extended so that the start of each scan line is eight byte aligned.

    Other common properties are the scanLineDirection(), and frameRate().
    Additionally a stream may have some additional type specific properties which are listed by the
    dynamicPropertyNames() function and can be accessed using the property(), and setProperty()
    functions.
*/

/*!
    \enum QVideoSurfaceFormat::Direction

    Enumerates the layout direction of video scan lines.

    \value TopToBottom Scan lines are arranged from the top of the frame to the bottom.
    \value BottomToTop Scan lines are arranged from the bottom of the frame to the top.
*/

/*!
    \enum QVideoSurfaceFormat::YCbCrColorSpace

    Enumerates the Y'CbCr color space of video frames.

    \value YCbCr_Undefined
    No color space is specified.

    \value YCbCr_BT601
    A Y'CbCr color space defined by ITU-R recommendation BT.601
    with Y value range from 16 to 235, and Cb/Cr range from 16 to 240.
    Used in standard definition video.

    \value YCbCr_BT709
    A Y'CbCr color space defined by ITU-R BT.709 with the same values range as YCbCr_BT601.  Used
    for HDTV.

    \value YCbCr_xvYCC601
    The BT.601 color space with the value range extended to 0 to 255.
    It is backward compatibile with BT.601 and uses values outside BT.601 range to represent a
    wider range of colors.

    \value YCbCr_xvYCC709
    The BT.709 color space with the value range extended to 0 to 255.

    \value YCbCr_JPEG
    The full range Y'CbCr color space used in JPEG files.
*/

/*!
    Constructs a null video stream format.
*/
QVideoSurfaceFormat::QVideoSurfaceFormat()
    : d(new QVideoSurfaceFormatPrivate)
{
    initResource();
}

/*!
    Contructs a description of stream which receives stream of \a type buffers with given frame
    \a size and pixel \a format.
*/
QVideoSurfaceFormat::QVideoSurfaceFormat(
        const QSize& size, QVideoSurfaceFormat::PixelFormat format)
    : d(new QVideoSurfaceFormatPrivate(size, format))
{
}

/*!
    Constructs a copy of \a other.
*/
QVideoSurfaceFormat::QVideoSurfaceFormat(const QVideoSurfaceFormat &other) = default;

/*!
    Assigns the values of \a other to this object.
*/
QVideoSurfaceFormat &QVideoSurfaceFormat::operator =(const QVideoSurfaceFormat &other) = default;

/*!
    Destroys a video stream description.
*/
QVideoSurfaceFormat::~QVideoSurfaceFormat() = default;

/*!
    Identifies if a video surface format has a valid pixel format and frame size.

    Returns true if the format is valid, and false otherwise.
*/
bool QVideoSurfaceFormat::isValid() const
{
    return d->pixelFormat != Format_Invalid && d->frameSize.isValid();
}

/*!
    Returns true if \a other is the same as this video format, and false if they are different.
*/
bool QVideoSurfaceFormat::operator ==(const QVideoSurfaceFormat &other) const
{
    return d == other.d || *d == *other.d;
}

/*!
    Returns true if \a other is different to this video format, and false if they are the same.
*/
bool QVideoSurfaceFormat::operator !=(const QVideoSurfaceFormat &other) const
{
    return d != other.d && !(*d == *other.d);
}

/*!
    Returns the pixel format of frames in a video stream.
*/
QVideoSurfaceFormat::PixelFormat QVideoSurfaceFormat::pixelFormat() const
{
    return d->pixelFormat;
}

/*!
    Returns the dimensions of frames in a video stream.

    \sa frameWidth(), frameHeight()
*/
QSize QVideoSurfaceFormat::frameSize() const
{
    return d->frameSize;
}

/*!
    Returns the width of frames in a video stream.

    \sa frameSize(), frameHeight()
*/
int QVideoSurfaceFormat::frameWidth() const
{
    return d->frameSize.width();
}

/*!
    Returns the height of frame in a video stream.
*/
int QVideoSurfaceFormat::frameHeight() const
{
    return d->frameSize.height();
}

int QVideoSurfaceFormat::nPlanes() const
{
    switch (d->pixelFormat) {
    case Format_Invalid:
    case Format_ARGB32:
    case Format_ARGB32_Premultiplied:
    case Format_RGB32:
    case Format_RGB24:
    case Format_RGB565:
    case Format_RGB555:
    case Format_ARGB8565_Premultiplied:
    case Format_BGRA32:
    case Format_BGRA32_Premultiplied:
    case Format_ABGR32:
    case Format_BGR32:
    case Format_BGR24:
    case Format_BGR565:
    case Format_BGR555:
    case Format_BGRA5658_Premultiplied:
    case Format_AYUV444:
    case Format_AYUV444_Premultiplied:
    case Format_YUV444:
    case Format_UYVY:
    case Format_YUYV:
    case Format_Y8:
    case Format_Y16:
    case Format_Jpeg:
        return 1;
    case Format_NV12:
    case Format_NV21:
    case Format_IMC2:
    case Format_IMC4:
    case Format_P010LE:
    case Format_P010BE:
    case Format_P016LE:
    case Format_P016BE:
        return 2;
    case Format_YUV420P:
    case Format_YUV422P:
    case Format_YV12:
    case Format_IMC1:
    case Format_IMC3:
        return 3;
    }
    return 0;
}

/*!
    Sets the size of frames in a video stream to \a size.

    This will reset the viewport() to fill the entire frame.
*/
void QVideoSurfaceFormat::setFrameSize(const QSize &size)
{
    d->frameSize = size;
    d->viewport = QRect(QPoint(0, 0), size);
}

/*!
    \overload

    Sets the \a width and \a height of frames in a video stream.

    This will reset the viewport() to fill the entire frame.
*/
void QVideoSurfaceFormat::setFrameSize(int width, int height)
{
    d->frameSize = QSize(width, height);
    d->viewport = QRect(0, 0, width, height);
}

/*!
    Returns the viewport of a video stream.

    The viewport is the region of a video frame that is actually displayed.

    By default the viewport covers an entire frame.
*/
QRect QVideoSurfaceFormat::viewport() const
{
    return d->viewport;
}

/*!
    Sets the viewport of a video stream to \a viewport.
*/
void QVideoSurfaceFormat::setViewport(const QRect &viewport)
{
    d->viewport = viewport;
}

/*!
    Returns the direction of scan lines.
*/
QVideoSurfaceFormat::Direction QVideoSurfaceFormat::scanLineDirection() const
{
    return d->scanLineDirection;
}

/*!
    Sets the \a direction of scan lines.
*/
void QVideoSurfaceFormat::setScanLineDirection(Direction direction)
{
    d->scanLineDirection = direction;
}

/*!
    Returns the frame rate of a video stream in frames per second.
*/
qreal QVideoSurfaceFormat::frameRate() const
{
    return d->frameRate;
}

/*!
    Sets the frame \a rate of a video stream in frames per second.
*/
void QVideoSurfaceFormat::setFrameRate(qreal rate)
{
    d->frameRate = rate;
}

/*!
    Returns the Y'CbCr color space of a video stream.
*/
QVideoSurfaceFormat::YCbCrColorSpace QVideoSurfaceFormat::yCbCrColorSpace() const
{
    return d->ycbcrColorSpace;
}

/*!
    Sets the Y'CbCr color \a space of a video stream.
    It is only used with raw YUV frame types.
*/
void QVideoSurfaceFormat::setYCbCrColorSpace(QVideoSurfaceFormat::YCbCrColorSpace space)
{
    d->ycbcrColorSpace = space;
}

/*!
    Returns \c true if the surface is mirrored around its vertical axis.
    This is typically needed for video frames coming from a front camera of a mobile device.

    \note The mirroring here differs from QImage::mirrored, as a vertically mirrored QImage
    will be mirrored around its x-axis.

    \since 5.11
 */
bool QVideoSurfaceFormat::isMirrored() const
{
    return d->mirrored;
}

/*!
    Sets if the surface is \a mirrored around its vertical axis.
    This is typically needed for video frames coming from a front camera of a mobile device.
    Default value is false.

    \note The mirroring here differs from QImage::mirrored, as a vertically mirrored QImage
    will be mirrored around its x-axis.

    \since 5.11
 */
void QVideoSurfaceFormat::setMirrored(bool mirrored)
{
    d->mirrored = mirrored;
}

/*!
    Returns a suggested size in pixels for the video stream.

    This is the same as the size of the viewport.
*/
QSize QVideoSurfaceFormat::sizeHint() const
{
    return d->viewport.size();
}

QString QVideoSurfaceFormat::vertexShaderFileName() const
{
    switch (d->pixelFormat) {
    case Format_Invalid:
    case Format_Jpeg:

    case Format_RGB24:
    case Format_RGB565:
    case Format_RGB555:
    case Format_ARGB8565_Premultiplied:
    case Format_BGR24:
    case Format_BGR565:
    case Format_BGR555:
    case Format_BGRA5658_Premultiplied:

    case Format_Y8:
    case Format_Y16:

    case Format_AYUV444:
    case Format_AYUV444_Premultiplied:
    case Format_YUV444:

    case Format_IMC1:
    case Format_IMC2:
    case Format_IMC3:
    case Format_IMC4:
        return QString();
    case Format_ARGB32:
    case Format_ARGB32_Premultiplied:
    case Format_RGB32:
    case Format_BGRA32:
    case Format_BGRA32_Premultiplied:
    case Format_ABGR32:
    case Format_BGR32:
        return QStringLiteral(":/qtmultimedia/shaders/rgba.vert.qsb");
    case Format_YUV420P:
    case Format_YUV422P:
    case Format_YV12:
    case Format_UYVY:
    case Format_YUYV:
    case Format_NV12:
    case Format_NV21:
    case Format_P010LE:
    case Format_P010BE:
    case Format_P016LE:
    case Format_P016BE:
        return QStringLiteral(":/qtmultimedia/shaders/yuv.vert.qsb");
    }
}

QString QVideoSurfaceFormat::fragmentShaderFileName() const
{
    switch (d->pixelFormat) {
    case Format_Invalid:
    case Format_Jpeg:

    case Format_RGB24:
    case Format_RGB565:
    case Format_RGB555:
    case Format_ARGB8565_Premultiplied:
    case Format_BGR24:
    case Format_BGR565:
    case Format_BGR555:
    case Format_BGRA5658_Premultiplied:

    case Format_Y8:
    case Format_Y16:

    case Format_AYUV444:
    case Format_AYUV444_Premultiplied:
    case Format_YUV444:

    case Format_IMC1:
    case Format_IMC2:
    case Format_IMC3:
    case Format_IMC4:
        return QString();
    case Format_ARGB32:
    case Format_ARGB32_Premultiplied:
    case Format_RGB32:
    case Format_BGRA32:
    case Format_BGRA32_Premultiplied:
    case Format_ABGR32:
    case Format_BGR32:
        return QStringLiteral(":/qtmultimedia/shaders/rgba.frag.qsb");
    case Format_YUV420P:
    case Format_YUV422P:
    case Format_YV12:
        return QStringLiteral(":/qtmultimedia/shaders/yuv_yv.frag.qsb");
    case Format_UYVY:
        return QStringLiteral(":/qtmultimedia/shaders/uyvy.frag.qsb");
    case Format_YUYV:
        return QStringLiteral(":/qtmultimedia/shaders/yuyv.frag.qsb");
    case Format_NV12:
        return QStringLiteral(":/qtmultimedia/shaders/nv12.frag.qsb");
    case Format_NV21:
        return QStringLiteral(":/qtmultimedia/shaders/nv21.frag.qsb");
    case Format_P010LE:
    case Format_P016LE:
        return QStringLiteral(":/qtmultimedia/shaders/p010le.frag.qsb");
    case Format_P010BE:
    case Format_P016BE:
        return QStringLiteral(":/qtmultimedia/shaders/p010be.frag.qsb");
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

QByteArray QVideoSurfaceFormat::uniformData(const QMatrix4x4 &transform, float opacity) const
{
    static constexpr float pw[3] = {};
    const float *planeWidth = pw;

    switch (d->pixelFormat) {
    case Format_Invalid:
    case Format_Jpeg:

    case Format_RGB24:
    case Format_RGB565:
    case Format_RGB555:
    case Format_ARGB8565_Premultiplied:
    case Format_BGR24:
    case Format_BGR565:
    case Format_BGR555:
    case Format_BGRA5658_Premultiplied:

    case Format_Y8:
    case Format_Y16:

    case Format_AYUV444:
    case Format_AYUV444_Premultiplied:
    case Format_YUV444:

    case Format_IMC1:
    case Format_IMC2:
    case Format_IMC3:
    case Format_IMC4:
        return QByteArray();
    case Format_ARGB32:
    case Format_ARGB32_Premultiplied:
    case Format_RGB32:
    case Format_BGRA32:
    case Format_BGRA32_Premultiplied:
    case Format_ABGR32:
    case Format_BGR32: {
        // { matrix4x4, opacity }
        QByteArray buf(16*4 + 4, Qt::Uninitialized);
        char *data = buf.data();
        memcpy(data, transform.constData(), 64);
        memcpy(data + 64, &opacity, 4);
        return buf;
    }
    case Format_YUV420P:
    case Format_YUV422P:
    case Format_YV12: {
        static constexpr float pw[] = { 1, 1, 1 };
        planeWidth = pw;
        break;
    }
    case Format_UYVY:
    case Format_YUYV: {
        static constexpr float pw[] = { 1, 1, 0 };
        planeWidth = pw;
        break;
    }
    case Format_NV12:
    case Format_NV21:
    case Format_P010LE:
    case Format_P010BE:
    case Format_P016LE:
    case Format_P016BE: {
        static constexpr float pw[] = { 1, 1, 0 };
        planeWidth = pw;
        break;
    }
    }
    // { matrix4x4, colorMatrix, opacity, planeWidth[3] }
    QByteArray buf(64*2 + 4 + 3*4, Qt::Uninitialized);
    char *data = buf.data();
    memcpy(data, transform.constData(), 64);
    memcpy(data + 64, colorMatrix(d->ycbcrColorSpace).constData(), 64);
    memcpy(data + 64 + 64, &opacity, 4);
    memcpy(data + 64 + 64 + 4, planeWidth, 3*4);
    return buf;
}


/*!
    Returns a video pixel format equivalent to an image \a format.  If there is no equivalent
    format QVideoFrame::InvalidType is returned instead.

    \note In general \l QImage does not handle YUV formats.

*/
QVideoSurfaceFormat::PixelFormat QVideoSurfaceFormat::pixelFormatFromImageFormat(QImage::Format format)
{
    switch (format) {
    case QImage::Format_RGB32:
    case QImage::Format_RGBX8888:
        return QVideoSurfaceFormat::Format_RGB32;
    case QImage::Format_ARGB32:
    case QImage::Format_RGBA8888:
        return QVideoSurfaceFormat::Format_ARGB32;
    case QImage::Format_ARGB32_Premultiplied:
    case QImage::Format_RGBA8888_Premultiplied:
        return QVideoSurfaceFormat::Format_ARGB32_Premultiplied;
    case QImage::Format_RGB16:
        return QVideoSurfaceFormat::Format_RGB565;
    case QImage::Format_ARGB8565_Premultiplied:
        return QVideoSurfaceFormat::Format_ARGB8565_Premultiplied;
    case QImage::Format_RGB555:
        return QVideoSurfaceFormat::Format_RGB555;
    case QImage::Format_RGB888:
        return QVideoSurfaceFormat::Format_RGB24;
    case QImage::Format_Grayscale8:
        return QVideoSurfaceFormat::Format_Y8;
    case QImage::Format_Grayscale16:
        return QVideoSurfaceFormat::Format_Y16;
    default:
        return QVideoSurfaceFormat::Format_Invalid;
    }
}

/*!
    Returns an image format equivalent to a video frame pixel \a format.  If there is no equivalent
    format QImage::Format_Invalid is returned instead.

    \note In general \l QImage does not handle YUV formats.

*/
QImage::Format QVideoSurfaceFormat::imageFormatFromPixelFormat(QVideoSurfaceFormat::PixelFormat format)
{
    switch (format) {
    case QVideoSurfaceFormat::Format_ARGB32:
        return QImage::Format_ARGB32;
    case QVideoSurfaceFormat::Format_ARGB32_Premultiplied:
        return QImage::Format_ARGB32_Premultiplied;
    case QVideoSurfaceFormat::Format_RGB32:
        return QImage::Format_RGB32;
    case QVideoSurfaceFormat::Format_RGB24:
        return QImage::Format_RGB888;
    case QVideoSurfaceFormat::Format_RGB565:
        return QImage::Format_RGB16;
    case QVideoSurfaceFormat::Format_RGB555:
        return QImage::Format_RGB555;
    case QVideoSurfaceFormat::Format_ARGB8565_Premultiplied:
        return QImage::Format_ARGB8565_Premultiplied;
    case QVideoSurfaceFormat::Format_Y8:
        return QImage::Format_Grayscale8;
    case QVideoSurfaceFormat::Format_Y16:
        return QImage::Format_Grayscale16;
    case QVideoSurfaceFormat::Format_ABGR32:
    case QVideoSurfaceFormat::Format_BGRA32:
    case QVideoSurfaceFormat::Format_BGRA32_Premultiplied:
    case QVideoSurfaceFormat::Format_BGR32:
    case QVideoSurfaceFormat::Format_BGR24:
    case QVideoSurfaceFormat::Format_BGR565:
    case QVideoSurfaceFormat::Format_BGR555:
    case QVideoSurfaceFormat::Format_BGRA5658_Premultiplied:
    case QVideoSurfaceFormat::Format_AYUV444:
    case QVideoSurfaceFormat::Format_AYUV444_Premultiplied:
    case QVideoSurfaceFormat::Format_YUV444:
    case QVideoSurfaceFormat::Format_YUV420P:
    case QVideoSurfaceFormat::Format_YUV422P:
    case QVideoSurfaceFormat::Format_YV12:
    case QVideoSurfaceFormat::Format_UYVY:
    case QVideoSurfaceFormat::Format_YUYV:
    case QVideoSurfaceFormat::Format_NV12:
    case QVideoSurfaceFormat::Format_NV21:
    case QVideoSurfaceFormat::Format_IMC1:
    case QVideoSurfaceFormat::Format_IMC2:
    case QVideoSurfaceFormat::Format_IMC3:
    case QVideoSurfaceFormat::Format_IMC4:
    case QVideoSurfaceFormat::Format_P010LE:
    case QVideoSurfaceFormat::Format_P010BE:
    case QVideoSurfaceFormat::Format_P016LE:
    case QVideoSurfaceFormat::Format_P016BE:
    case QVideoSurfaceFormat::Format_Jpeg:
    case QVideoSurfaceFormat::Format_Invalid:
        return QImage::Format_Invalid;
    }
    return QImage::Format_Invalid;
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, QVideoSurfaceFormat::YCbCrColorSpace cs)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace();
    switch (cs) {
        case QVideoSurfaceFormat::YCbCr_BT601:
            dbg << "YCbCr_BT601";
            break;
        case QVideoSurfaceFormat::YCbCr_BT709:
            dbg << "YCbCr_BT709";
            break;
        case QVideoSurfaceFormat::YCbCr_JPEG:
            dbg << "YCbCr_JPEG";
            break;
        case QVideoSurfaceFormat::YCbCr_xvYCC601:
            dbg << "YCbCr_xvYCC601";
            break;
        case QVideoSurfaceFormat::YCbCr_xvYCC709:
            dbg << "YCbCr_xvYCC709";
            break;
        default:
            dbg << "YCbCr_Undefined";
            break;
    }
    return dbg;
}

QDebug operator<<(QDebug dbg, QVideoSurfaceFormat::Direction dir)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace();
    switch (dir) {
        case QVideoSurfaceFormat::BottomToTop:
            dbg << "BottomToTop";
            break;
        case QVideoSurfaceFormat::TopToBottom:
            dbg << "TopToBottom";
            break;
    }
    return dbg;
}

QDebug operator<<(QDebug dbg, const QVideoSurfaceFormat &f)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace();
    dbg << "QVideoSurfaceFormat(" << f.pixelFormat() << ", " << f.frameSize()
        << ", viewport=" << f.viewport()
        <<  ", yCbCrColorSpace=" << f.yCbCrColorSpace()
        << ')'
        << "\n    pixel format=" << f.pixelFormat()
        << "\n    frame size=" << f.frameSize()
        << "\n    viewport=" << f.viewport()
        << "\n    yCbCrColorSpace=" << f.yCbCrColorSpace()
        << "\n    frameRate=" << f.frameRate()
        << "\n    mirrored=" << f.isMirrored();

    return dbg;
}

QDebug operator<<(QDebug dbg, QVideoSurfaceFormat::PixelFormat pf)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace();
    switch (pf) {
    case QVideoSurfaceFormat::Format_Invalid:
        return dbg << "Format_Invalid";
    case QVideoSurfaceFormat::Format_ARGB32:
        return dbg << "Format_ARGB32";
    case QVideoSurfaceFormat::Format_ARGB32_Premultiplied:
        return dbg << "Format_ARGB32_Premultiplied";
    case QVideoSurfaceFormat::Format_RGB32:
        return dbg << "Format_RGB32";
    case QVideoSurfaceFormat::Format_RGB24:
        return dbg << "Format_RGB24";
    case QVideoSurfaceFormat::Format_RGB565:
        return dbg << "Format_RGB565";
    case QVideoSurfaceFormat::Format_RGB555:
        return dbg << "Format_RGB555";
    case QVideoSurfaceFormat::Format_ARGB8565_Premultiplied:
        return dbg << "Format_ARGB8565_Premultiplied";
    case QVideoSurfaceFormat::Format_BGRA32:
        return dbg << "Format_BGRA32";
    case QVideoSurfaceFormat::Format_BGRA32_Premultiplied:
        return dbg << "Format_BGRA32_Premultiplied";
    case QVideoSurfaceFormat::Format_ABGR32:
        return dbg << "Format_ABGR32";
    case QVideoSurfaceFormat::Format_BGR32:
        return dbg << "Format_BGR32";
    case QVideoSurfaceFormat::Format_BGR24:
        return dbg << "Format_BGR24";
    case QVideoSurfaceFormat::Format_BGR565:
        return dbg << "Format_BGR565";
    case QVideoSurfaceFormat::Format_BGR555:
        return dbg << "Format_BGR555";
    case QVideoSurfaceFormat::Format_BGRA5658_Premultiplied:
        return dbg << "Format_BGRA5658_Premultiplied";
    case QVideoSurfaceFormat::Format_AYUV444:
        return dbg << "Format_AYUV444";
    case QVideoSurfaceFormat::Format_AYUV444_Premultiplied:
        return dbg << "Format_AYUV444_Premultiplied";
    case QVideoSurfaceFormat::Format_YUV444:
        return dbg << "Format_YUV444";
    case QVideoSurfaceFormat::Format_YUV420P:
        return dbg << "Format_YUV420P";
    case QVideoSurfaceFormat::Format_YUV422P:
        return dbg << "Format_YUV422P";
    case QVideoSurfaceFormat::Format_YV12:
        return dbg << "Format_YV12";
    case QVideoSurfaceFormat::Format_UYVY:
        return dbg << "Format_UYVY";
    case QVideoSurfaceFormat::Format_YUYV:
        return dbg << "Format_YUYV";
    case QVideoSurfaceFormat::Format_NV12:
        return dbg << "Format_NV12";
    case QVideoSurfaceFormat::Format_NV21:
        return dbg << "Format_NV21";
    case QVideoSurfaceFormat::Format_IMC1:
        return dbg << "Format_IMC1";
    case QVideoSurfaceFormat::Format_IMC2:
        return dbg << "Format_IMC2";
    case QVideoSurfaceFormat::Format_IMC3:
        return dbg << "Format_IMC3";
    case QVideoSurfaceFormat::Format_IMC4:
        return dbg << "Format_IMC4";
    case QVideoSurfaceFormat::Format_Y8:
        return dbg << "Format_Y8";
    case QVideoSurfaceFormat::Format_Y16:
        return dbg << "Format_Y16";
    case QVideoSurfaceFormat::Format_P010LE:
        return dbg << "Format_P010LE";
    case QVideoSurfaceFormat::Format_P010BE:
        return dbg << "Format_P010BE";
    case QVideoSurfaceFormat::Format_P016LE:
        return dbg << "Format_P016LE";
    case QVideoSurfaceFormat::Format_P016BE:
        return dbg << "Format_P016BE";
    case QVideoSurfaceFormat::Format_Jpeg:
        return dbg << "Format_Jpeg";

    default:
        return dbg << QString(QLatin1String("UserType(%1)" )).arg(int(pf)).toLatin1().constData();
    }
}
#endif

QT_END_NAMESPACE
