// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qvideoframe.h"

#include "qvideotexturehelper_p.h"
#include "qmemoryvideobuffer_p.h"
#include "qvideoframeconverter_p.h"
#include "qvideoframeformat.h"
#include "qpainter.h"
#include <qtextlayout.h>

#include <qimage.h>
#include <qmutex.h>
#include <qpair.h>
#include <qsize.h>
#include <qvariant.h>
#include <rhi/qrhi.h>

#include <mutex>

#include <QDebug>

QT_BEGIN_NAMESPACE

class QVideoFramePrivate : public QSharedData
{
public:
    QVideoFramePrivate() = default;
    QVideoFramePrivate(const QVideoFrameFormat &format)
        : format(format)
    {
    }

    ~QVideoFramePrivate()
    {
        delete buffer;
    }

    qint64 startTime = -1;
    qint64 endTime = -1;
    QAbstractVideoBuffer::MapData mapData;
    QVideoFrameFormat format;
    QAbstractVideoBuffer *buffer = nullptr;
    int mappedCount = 0;
    QMutex mapMutex;
    QString subtitleText;
    QVideoFrame::RotationAngle rotationAngle = QVideoFrame::Rotation0;
    bool mirrored = false;
    QImage image;
    std::once_flag imageOnceFlag;

private:
    Q_DISABLE_COPY(QVideoFramePrivate)
};

QT_DEFINE_QESDP_SPECIALIZATION_DTOR(QVideoFramePrivate);

/*!
    \class QVideoFrame
    \brief The QVideoFrame class represents a frame of video data.
    \inmodule QtMultimedia

    \ingroup multimedia
    \ingroup multimedia_video

    A QVideoFrame encapsulates the pixel data of a video frame, and information about the frame.

    Video frames can come from several places - decoded \l {QMediaPlayer}{media}, a
    \l {QCamera}{camera}, or generated programmatically.  The way pixels are described in these
    frames can vary greatly, and some pixel formats offer greater compression opportunities at
    the expense of ease of use.

    The pixel contents of a video frame can be mapped to memory using the map() function. After
    a successful call to map(), the video data can be accessed through various functions. Some of
    the YUV pixel formats provide the data in several planes. The planeCount() method will return
    the amount of planes that being used.

    While mapped, the video data of each plane can accessed using the bits() function, which
    returns a pointer to a buffer.  The size of this buffer is given by the mappedBytes() function,
    and the size of each line is given by bytesPerLine().  The return value of the handle()
    function may also be used to access frame data using the internal buffer's native APIs
    (for example - an OpenGL texture handle).

    A video frame can also have timestamp information associated with it.  These timestamps can be
    used to determine when to start and stop displaying the frame.

    QVideoFrame objects can consume a significant amount of memory or system resources and
    should not be held for longer than required by the application.

    \note Since video frames can be expensive to copy, QVideoFrame is explicitly shared, so any
    change made to a video frame will also apply to any copies.
*/


/*!
    Constructs a null video frame.
*/
QVideoFrame::QVideoFrame()
{
}

/*!
    \internal
    Constructs a video frame from a \a buffer with the given pixel \a format and \a size in pixels.

    \note This doesn't increment the reference count of the video buffer.
*/
QVideoFrame::QVideoFrame(QAbstractVideoBuffer *buffer, const QVideoFrameFormat &format)
    : d(new QVideoFramePrivate(format))
{
    d->buffer = buffer;
}

/*!
    \internal
*/
QAbstractVideoBuffer *QVideoFrame::videoBuffer() const
{
    return d ? d->buffer : nullptr;
}

/*!
    Constructs a video frame of the given pixel \a format.

*/
QVideoFrame::QVideoFrame(const QVideoFrameFormat &format)
    : d(new QVideoFramePrivate(format))
{
    auto *textureDescription = QVideoTextureHelper::textureDescription(format.pixelFormat());
    qsizetype bytes = textureDescription->bytesForSize(format.frameSize());
    if (bytes > 0) {
        QByteArray data;
        data.resize(bytes);

        // Check the memory was successfully allocated.
        if (!data.isEmpty())
            d->buffer = new QMemoryVideoBuffer(data, textureDescription->strideForWidth(format.frameWidth()));
    }
}

/*!
    Constructs a shallow copy of \a other.  Since QVideoFrame is
    explicitly shared, these two instances will reflect the same frame.

*/
QVideoFrame::QVideoFrame(const QVideoFrame &other) = default;

/*!
    \fn  QVideoFrame::QVideoFrame(QVideoFrame &&other)

    Constructs a QVideoFrame by moving from \a other.
*/

/*!
    \fn void QVideoFrame::swap(QVideoFrame &other) noexcept

    Swaps the current video frame with \a other.
*/

/*!
    \fn  QVideoFrame &QVideoFrame::operator=(QVideoFrame &&other)

    Moves \a other into this QVideoFrame.
*/

/*!
    Assigns the contents of \a other to this video frame.  Since QVideoFrame is
    explicitly shared, these two instances will reflect the same frame.

*/
QVideoFrame &QVideoFrame::operator =(const QVideoFrame &other) = default;

/*!
  \return \c true if this QVideoFrame and \a other reflect the same frame.
 */
bool QVideoFrame::operator==(const QVideoFrame &other) const
{
    // Due to explicit sharing we just compare the QSharedData which in turn compares the pointers.
    return d == other.d;
}

/*!
  \return \c true if this QVideoFrame and \a other do not reflect the same frame.
 */
bool QVideoFrame::operator!=(const QVideoFrame &other) const
{
    return d != other.d;
}

/*!
    Destroys a video frame.
*/
QVideoFrame::~QVideoFrame() = default;

/*!
    Identifies whether a video frame is valid.

    An invalid frame has no video buffer associated with it.

    Returns true if the frame is valid, and false if it is not.
*/
bool QVideoFrame::isValid() const
{
    return (d && d->buffer) && d->format.pixelFormat() != QVideoFrameFormat::Format_Invalid;
}

/*!
    Returns the pixel format of this video frame.
*/
QVideoFrameFormat::PixelFormat QVideoFrame::pixelFormat() const
{
    return d ? d->format.pixelFormat() : QVideoFrameFormat::Format_Invalid;
}

/*!
    Returns the surface format of this video frame.
*/
QVideoFrameFormat QVideoFrame::surfaceFormat() const
{
    return d ? d->format : QVideoFrameFormat{};
}

/*!
    Returns the type of a video frame's handle.

    The handle type could either be NoHandle, meaning that the frame is memory
    based, or a RHI texture.
*/
QVideoFrame::HandleType QVideoFrame::handleType() const
{
    return (d && d->buffer) ? d->buffer->handleType() : QVideoFrame::NoHandle;
}

/*!
    Returns the dimensions of a video frame.
*/
QSize QVideoFrame::size() const
{
    return d ? d->format.frameSize() : QSize();
}

/*!
    Returns the width of a video frame.
*/
int QVideoFrame::width() const
{
    return size().width();
}

/*!
    Returns the height of a video frame.
*/
int QVideoFrame::height() const
{
    return size().height();
}

/*!
    Identifies if a video frame's contents are currently mapped to system memory.

    This is a convenience function which checks that the \l {QVideoFrame::MapMode}{MapMode}
    of the frame is not equal to QVideoFrame::NotMapped.

    Returns true if the contents of the video frame are mapped to system memory, and false
    otherwise.

    \sa mapMode(), QVideoFrame::MapMode
*/

bool QVideoFrame::isMapped() const
{
    return d && d->buffer && d->buffer->mapMode() != QVideoFrame::NotMapped;
}

/*!
    Identifies if the mapped contents of a video frame will be persisted when the frame is unmapped.

    This is a convenience function which checks if the \l {QVideoFrame::MapMode}{MapMode}
    contains the QVideoFrame::WriteOnly flag.

    Returns true if the video frame will be updated when unmapped, and false otherwise.

    \note The result of altering the data of a frame that is mapped in read-only mode is undefined.
    Depending on the buffer implementation the changes may be persisted, or worse alter a shared
    buffer.

    \sa mapMode(), QVideoFrame::MapMode
*/
bool QVideoFrame::isWritable() const
{
    return d && d->buffer && (d->buffer->mapMode() & QVideoFrame::WriteOnly);
}

/*!
    Identifies if the mapped contents of a video frame were read from the frame when it was mapped.

    This is a convenience function which checks if the \l {QVideoFrame::MapMode}{MapMode}
    contains the QVideoFrame::WriteOnly flag.

    Returns true if the contents of the mapped memory were read from the video frame, and false
    otherwise.

    \sa mapMode(), QVideoFrame::MapMode
*/
bool QVideoFrame::isReadable() const
{
    return d && d->buffer && (d->buffer->mapMode() & QVideoFrame::ReadOnly);
}

/*!
    Returns the mode a video frame was mapped to system memory in.

    \sa map(), QVideoFrame::MapMode
*/
QVideoFrame::MapMode QVideoFrame::mapMode() const
{
    return (d && d->buffer) ? d->buffer->mapMode() : QVideoFrame::NotMapped;
}

/*!
    Maps the contents of a video frame to system (CPU addressable) memory.

    In some cases the video frame data might be stored in video memory or otherwise inaccessible
    memory, so it is necessary to map a frame before accessing the pixel data.  This may involve
    copying the contents around, so avoid mapping and unmapping unless required.

    The map \a mode indicates whether the contents of the mapped memory should be read from and/or
    written to the frame.  If the map mode includes the \c QVideoFrame::ReadOnly flag the
    mapped memory will be populated with the content of the video frame when initially mapped.  If the map
    mode includes the \c QVideoFrame::WriteOnly flag the content of the possibly modified
    mapped memory will be written back to the frame when unmapped.

    While mapped the contents of a video frame can be accessed directly through the pointer returned
    by the bits() function.

    When access to the data is no longer needed, be sure to call the unmap() function to release the
    mapped memory and possibly update the video frame contents.

    If the video frame has been mapped in read only mode, it is permissible to map it
    multiple times in read only mode (and unmap it a corresponding number of times). In all
    other cases it is necessary to unmap the frame first before mapping a second time.

    \note Writing to memory that is mapped as read-only is undefined, and may result in changes
    to shared data or crashes.

    Returns true if the frame was mapped to memory in the given \a mode and false otherwise.

    \sa unmap(), mapMode(), bits()
*/
bool QVideoFrame::map(QVideoFrame::MapMode mode)
{

    if (!d || !d->buffer)
        return false;

    QMutexLocker lock(&d->mapMutex);
    if (mode == QVideoFrame::NotMapped)
        return false;

    if (d->mappedCount > 0) {
        //it's allowed to map the video frame multiple times in read only mode
        if (d->buffer->mapMode() == QVideoFrame::ReadOnly
                && mode == QVideoFrame::ReadOnly) {
            d->mappedCount++;
            return true;
        }

        return false;
    }

    Q_ASSERT(d->mapData.data[0] == nullptr);
    Q_ASSERT(d->mapData.bytesPerLine[0] == 0);
    Q_ASSERT(d->mapData.nPlanes == 0);
    Q_ASSERT(d->mapData.size[0] == 0);

    d->mapData = d->buffer->map(mode);
    if (d->mapData.nPlanes == 0)
        return false;

    if (d->mapData.nPlanes == 1) {
        auto pixelFmt = d->format.pixelFormat();
        // If the plane count is 1 derive the additional planes for planar formats.
        switch (pixelFmt) {
        case QVideoFrameFormat::Format_Invalid:
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
        case QVideoFrameFormat::Format_AYUV:
        case QVideoFrameFormat::Format_AYUV_Premultiplied:
        case QVideoFrameFormat::Format_UYVY:
        case QVideoFrameFormat::Format_YUYV:
        case QVideoFrameFormat::Format_Y8:
        case QVideoFrameFormat::Format_Y16:
        case QVideoFrameFormat::Format_Jpeg:
        case QVideoFrameFormat::Format_SamplerExternalOES:
        case QVideoFrameFormat::Format_SamplerRect:
            // Single plane or opaque format.
            break;
        case QVideoFrameFormat::Format_YUV420P:
        case QVideoFrameFormat::Format_YUV420P10:
        case QVideoFrameFormat::Format_YUV422P:
        case QVideoFrameFormat::Format_YV12: {
            // The UV stride is usually half the Y stride and is 32-bit aligned.
            // However it's not always the case, at least on Windows where the
            // UV planes are sometimes not aligned.
            // We calculate the stride using the UV byte count to always
            // have a correct stride.
            const int height = this->height();
            const int yStride = d->mapData.bytesPerLine[0];
            const int uvHeight = pixelFmt == QVideoFrameFormat::Format_YUV422P ? height : height / 2;
            const int uvStride = (d->mapData.size[0] - (yStride * height)) / uvHeight / 2;

            // Three planes, the second and third vertically (and horizontally for other than Format_YUV422P formats) subsampled.
            d->mapData.nPlanes = 3;
            d->mapData.bytesPerLine[2] = d->mapData.bytesPerLine[1] = uvStride;
            d->mapData.size[0] = yStride * height;
            d->mapData.size[1] = uvStride * uvHeight;
            d->mapData.size[2] = uvStride * uvHeight;
            d->mapData.data[1] = d->mapData.data[0] + d->mapData.size[0];
            d->mapData.data[2] = d->mapData.data[1] + d->mapData.size[1];
            break;
        }
        case QVideoFrameFormat::Format_NV12:
        case QVideoFrameFormat::Format_NV21:
        case QVideoFrameFormat::Format_IMC2:
        case QVideoFrameFormat::Format_IMC4:
        case QVideoFrameFormat::Format_P010:
        case QVideoFrameFormat::Format_P016: {
            // Semi planar, Full resolution Y plane with interleaved subsampled U and V planes.
            d->mapData.nPlanes = 2;
            d->mapData.bytesPerLine[1] = d->mapData.bytesPerLine[0];
            int size = d->mapData.size[0];
            d->mapData.size[0] = (d->mapData.bytesPerLine[0] * height());
            d->mapData.size[1] = size - d->mapData.size[0];
            d->mapData.data[1] = d->mapData.data[0] + d->mapData.size[0];
            break;
        }
        case QVideoFrameFormat::Format_IMC1:
        case QVideoFrameFormat::Format_IMC3: {
            // Three planes, the second and third vertically and horizontally subsumpled,
            // but with lines padded to the width of the first plane.
            d->mapData.nPlanes = 3;
            d->mapData.bytesPerLine[2] = d->mapData.bytesPerLine[1] = d->mapData.bytesPerLine[0];
            d->mapData.size[0] = (d->mapData.bytesPerLine[0] * height());
            d->mapData.size[1] = (d->mapData.bytesPerLine[0] * height() / 2);
            d->mapData.size[2] = (d->mapData.bytesPerLine[0] * height() / 2);
            d->mapData.data[1] = d->mapData.data[0] + d->mapData.size[0];
            d->mapData.data[2] = d->mapData.data[1] + d->mapData.size[1];
            break;
        }
        }
    }

    d->mappedCount++;
    return true;
}

/*!
    Releases the memory mapped by the map() function.

    If the \l {QVideoFrame::MapMode}{MapMode} included the QVideoFrame::WriteOnly
    flag this will persist the current content of the mapped memory to the video frame.

    unmap() should not be called if map() function failed.

    \sa map()
*/
void QVideoFrame::unmap()
{
    if (!d || !d->buffer)
        return;

    QMutexLocker lock(&d->mapMutex);

    if (d->mappedCount == 0) {
        qWarning() << "QVideoFrame::unmap() was called more times then QVideoFrame::map()";
        return;
    }

    d->mappedCount--;

    if (d->mappedCount == 0) {
        d->mapData = {};
        d->buffer->unmap();
    }
}

/*!
    Returns the number of bytes in a scan line of a \a plane.

    This value is only valid while the frame data is \l {map()}{mapped}.

    \sa bits(), map(), mappedBytes(), planeCount()
    \since 5.4
*/

int QVideoFrame::bytesPerLine(int plane) const
{
    if (!d)
        return 0;
    return plane >= 0 && plane < d->mapData.nPlanes ? d->mapData.bytesPerLine[plane] : 0;
}

/*!
    Returns a pointer to the start of the frame data buffer for a \a plane.

    This value is only valid while the frame data is \l {map()}{mapped}.

    Changes made to data accessed via this pointer (when mapped with write access)
    are only guaranteed to have been persisted when unmap() is called and when the
    buffer has been mapped for writing.

    \sa map(), mappedBytes(), bytesPerLine(), planeCount()
    \since 5.4
*/
uchar *QVideoFrame::bits(int plane)
{
    if (!d)
        return nullptr;
    return plane >= 0 && plane < d->mapData.nPlanes ? d->mapData.data[plane] : nullptr;
}

/*!
    Returns a pointer to the start of the frame data buffer for a \a plane.

    This value is only valid while the frame data is \l {map()}{mapped}.

    If the buffer was not mapped with read access, the contents of this
    buffer will initially be uninitialized.

    \sa map(), mappedBytes(), bytesPerLine(), planeCount()
    \since 5.4
*/
const uchar *QVideoFrame::bits(int plane) const
{
    if (!d)
        return nullptr;
    return plane >= 0 && plane < d->mapData.nPlanes ?  d->mapData.data[plane] : nullptr;
}

/*!
    Returns the number of bytes occupied by plane \a plane of the mapped frame data.

    This value is only valid while the frame data is \l {map()}{mapped}.

    \sa map()
*/
int QVideoFrame::mappedBytes(int plane) const
{
    if (!d)
        return 0;
    return plane >= 0 && plane < d->mapData.nPlanes ? d->mapData.size[plane] : 0;
}

/*!
    Returns the number of planes in the video frame.

    \sa map()
    \since 5.4
*/

int QVideoFrame::planeCount() const
{
    if (!d)
        return 0;
    return d->format.planeCount();
}

/*!
    Returns the presentation time (in microseconds) when the frame should be displayed.

    An invalid time is represented as -1.

*/
qint64 QVideoFrame::startTime() const
{
    if (!d)
        return -1;
    return d->startTime;
}

/*!
    Sets the presentation \a time (in microseconds) when the frame should initially be displayed.

    An invalid time is represented as -1.

*/
void QVideoFrame::setStartTime(qint64 time)
{
    if (!d)
        return;
    d->startTime = time;
}

/*!
    Returns the presentation time (in microseconds) when a frame should stop being displayed.

    An invalid time is represented as -1.

*/
qint64 QVideoFrame::endTime() const
{
    if (!d)
        return -1;
    return d->endTime;
}

/*!
    Sets the presentation \a time (in microseconds) when a frame should stop being displayed.

    An invalid time is represented as -1.

*/
void QVideoFrame::setEndTime(qint64 time)
{
    if (!d)
        return;
    d->endTime = time;
}

/*!
    \enum QVideoFrame::RotationAngle

    The angle of the clockwise rotation that should be applied to a video
    frame before displaying.

    \value Rotation0 No rotation required, the frame has correct orientation
    \value Rotation90 The frame should be rotated by 90 degrees
    \value Rotation180 The frame should be rotated by 180 degrees
    \value Rotation270 The frame should be rotated by 270 degrees
*/

/*!
    Sets the \a angle the frame should be rotated clockwise before displaying.
*/
void QVideoFrame::setRotationAngle(QVideoFrame::RotationAngle angle)
{
    if (d)
        d->rotationAngle = angle;
}

/*!
    Returns the angle the frame should be rotated clockwise before displaying.
 */
QVideoFrame::RotationAngle QVideoFrame::rotationAngle() const
{
    return d ? d->rotationAngle : Rotation0;
}

/*!
    Sets the \a mirrored flag for the frame.
*/
void QVideoFrame::setMirrored(bool mirrored)
{
    if (d)
        d->mirrored = mirrored;
}

/*!
    Returns whether the frame should be mirrored before displaying.
*/
bool QVideoFrame::mirrored() const
{
    return d && d->mirrored;
}

/*!
    Based on the pixel format converts current video frame to image.
    \since 5.15
*/
QImage QVideoFrame::toImage() const
{
    if (!isValid())
        return {};

    std::call_once(d->imageOnceFlag, [this]() {
        const bool mirrorY = surfaceFormat().scanLineDirection() != QVideoFrameFormat::TopToBottom;
        d->image = qImageFromVideoFrame(*this, rotationAngle(), mirrored(), mirrorY);
    });

    return d->image;
}

/*!
    Returns the subtitle text that should be rendered together with this video frame.
*/
QString QVideoFrame::subtitleText() const
{
    return d ? d->subtitleText : QString();
}

/*!
    Sets the subtitle text that should be rendered together with this video frame to \a text.
*/
void QVideoFrame::setSubtitleText(const QString &text)
{
    if (!d)
        return;
    d->subtitleText = text;
}

/*!
    Uses a QPainter, \a{painter}, to render this QVideoFrame to \a rect.
    The PaintOptions \a options can be used to specify a background color and
    how \a rect should be filled with the video.

    \note that rendering will usually happen without hardware acceleration when
    using this method.
*/
void QVideoFrame::paint(QPainter *painter, const QRectF &rect, const PaintOptions &options)
{
    if (!isValid()) {
        painter->fillRect(rect, options.backgroundColor);
        return;
    }

    QRectF targetRect = rect;
    QSizeF size = this->size();
    if (rotationAngle() % 180)
        size.transpose();

    size.scale(targetRect.size(), options.aspectRatioMode);

    if (options.aspectRatioMode == Qt::KeepAspectRatio) {
        targetRect = QRect(0, 0, size.width(), size.height());
        targetRect.moveCenter(rect.center());
        // we might not be drawing every pixel, fill the leftovers black
        if (options.backgroundColor != Qt::transparent && rect != targetRect) {
            if (targetRect.top() > rect.top()) {
                QRectF top(rect.left(), rect.top(), rect.width(), targetRect.top() - rect.top());
                painter->fillRect(top, Qt::black);
            }
            if (targetRect.left() > rect.left()) {
                QRectF top(rect.left(), targetRect.top(), targetRect.left() - rect.left(), targetRect.height());
                painter->fillRect(top, Qt::black);
            }
            if (targetRect.right() < rect.right()) {
                QRectF top(targetRect.right(), targetRect.top(), rect.right() - targetRect.right(), targetRect.height());
                painter->fillRect(top, Qt::black);
            }
            if (targetRect.bottom() < rect.bottom()) {
                QRectF top(rect.left(), targetRect.bottom(), rect.width(), rect.bottom() - targetRect.bottom());
                painter->fillRect(top, Qt::black);
            }
        }
    }

    if (map(QVideoFrame::ReadOnly)) {
        const QTransform oldTransform = painter->transform();
        QTransform transform = oldTransform;
        transform.translate(targetRect.center().x() - size.width()/2,
                            targetRect.center().y() - size.height()/2);
        painter->setTransform(transform);
        QImage image = toImage();
        painter->drawImage({{}, size}, image, {{},image.size()});
        painter->setTransform(oldTransform);

        unmap();
    } else if (isValid()) {
        // #### error handling
    } else {
        painter->fillRect(rect, Qt::black);
    }

    if ((options.paintFlags & PaintOptions::DontDrawSubtitles) || d->subtitleText.isEmpty())
        return;

    // draw subtitles
    auto text = d->subtitleText;
    text.replace(QLatin1Char('\n'), QChar::LineSeparator);

    QVideoTextureHelper::SubtitleLayout layout;
    layout.update(targetRect.size().toSize(), this->subtitleText());
    layout.draw(painter, targetRect.topLeft());
}

#ifndef QT_NO_DEBUG_STREAM
static QString qFormatTimeStamps(qint64 start, qint64 end)
{
    // Early out for invalid.
    if (start < 0)
        return QLatin1String("[no timestamp]");

    bool onlyOne = (start == end);

    // [hh:]mm:ss.ms
    const int s_millis = start % 1000000;
    start /= 1000000;
    const int s_seconds = start % 60;
    start /= 60;
    const int s_minutes = start % 60;
    start /= 60;

    if (onlyOne) {
        if (start > 0)
            return QString::fromLatin1("@%1:%2:%3.%4")
                    .arg(start, 1, 10, QLatin1Char('0'))
                    .arg(s_minutes, 2, 10, QLatin1Char('0'))
                    .arg(s_seconds, 2, 10, QLatin1Char('0'))
                    .arg(s_millis, 2, 10, QLatin1Char('0'));
        return QString::fromLatin1("@%1:%2.%3")
                .arg(s_minutes, 2, 10, QLatin1Char('0'))
                .arg(s_seconds, 2, 10, QLatin1Char('0'))
                .arg(s_millis, 2, 10, QLatin1Char('0'));
    }

    if (end == -1) {
        // Similar to start-start, except it means keep displaying it?
        if (start > 0)
            return QString::fromLatin1("%1:%2:%3.%4 - forever")
                    .arg(start, 1, 10, QLatin1Char('0'))
                    .arg(s_minutes, 2, 10, QLatin1Char('0'))
                    .arg(s_seconds, 2, 10, QLatin1Char('0'))
                    .arg(s_millis, 2, 10, QLatin1Char('0'));
        return QString::fromLatin1("%1:%2.%3 - forever")
                .arg(s_minutes, 2, 10, QLatin1Char('0'))
                .arg(s_seconds, 2, 10, QLatin1Char('0'))
                .arg(s_millis, 2, 10, QLatin1Char('0'));
    }

    const int e_millis = end % 1000000;
    end /= 1000000;
    const int e_seconds = end % 60;
    end /= 60;
    const int e_minutes = end % 60;
    end /= 60;

    if (start > 0 || end > 0)
        return QString::fromLatin1("%1:%2:%3.%4 - %5:%6:%7.%8")
                .arg(start, 1, 10, QLatin1Char('0'))
                .arg(s_minutes, 2, 10, QLatin1Char('0'))
                .arg(s_seconds, 2, 10, QLatin1Char('0'))
                .arg(s_millis, 2, 10, QLatin1Char('0'))
                .arg(end, 1, 10, QLatin1Char('0'))
                .arg(e_minutes, 2, 10, QLatin1Char('0'))
                .arg(e_seconds, 2, 10, QLatin1Char('0'))
                .arg(e_millis, 2, 10, QLatin1Char('0'));
    return QString::fromLatin1("%1:%2.%3 - %4:%5.%6")
            .arg(s_minutes, 2, 10, QLatin1Char('0'))
            .arg(s_seconds, 2, 10, QLatin1Char('0'))
            .arg(s_millis, 2, 10, QLatin1Char('0'))
            .arg(e_minutes, 2, 10, QLatin1Char('0'))
            .arg(e_seconds, 2, 10, QLatin1Char('0'))
            .arg(e_millis, 2, 10, QLatin1Char('0'));
}

QDebug operator<<(QDebug dbg, QVideoFrame::HandleType type)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace();
    switch (type) {
    case QVideoFrame::NoHandle:
        return dbg << "NoHandle";
    case QVideoFrame::RhiTextureHandle:
        return dbg << "RhiTextureHandle";
    }
    return dbg;
}

QDebug operator<<(QDebug dbg, const QVideoFrame& f)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace();
    dbg << "QVideoFrame(" << f.size() << ", "
               << f.pixelFormat() << ", "
               << f.handleType() << ", "
               << f.mapMode() << ", "
               << qFormatTimeStamps(f.startTime(), f.endTime()).toLatin1().constData();
    dbg << ')';
    return dbg;
}
#endif

QT_END_NAMESPACE

