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

QT_BEGIN_NAMESPACE

class QVideoSurfaceFormatPrivate : public QSharedData
{
public:
    QVideoSurfaceFormatPrivate() = default;

    QVideoSurfaceFormatPrivate(
            const QSize &size,
            QVideoFrame::PixelFormat format)
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

    QVideoFrame::PixelFormat pixelFormat = QVideoFrame::Format_Invalid;
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
}

/*!
    Contructs a description of stream which receives stream of \a type buffers with given frame
    \a size and pixel \a format.
*/
QVideoSurfaceFormat::QVideoSurfaceFormat(
        const QSize& size, QVideoFrame::PixelFormat format)
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
    return d->pixelFormat != QVideoFrame::Format_Invalid && d->frameSize.isValid();
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
QVideoFrame::PixelFormat QVideoSurfaceFormat::pixelFormat() const
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
#endif

QT_END_NAMESPACE
