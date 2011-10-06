/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

//TESTED_COMPONENT=src/multimedia

#include "qabstractvideosurface.h"

#include "qvideosurfaceformat.h"

#include <QtCore/qvariant.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_METATYPE(QVideoSurfaceFormat)
Q_DECLARE_METATYPE(QAbstractVideoSurface::Error)

/*!
    \class QAbstractVideoSurface
    \brief The QAbstractVideoSurface class is a base class for video presentation surfaces.
    \since 1.0
    \inmodule QtMultimedia

    A video surface presents a continuous stream of identically formatted frames, where the format
    of each frame is compatible with a stream format supplied when starting a presentation.

    The QAbstractVideoSurface class defines the standard interface that video producers use to
    inter-operate with video presentation surfaces.  It is not supposed to be instantiated directly.
    Instead, you should subclass it to create new video surfaces.

    A list of pixel formats a surface can present is given by the supportedPixelFormats() function,
    and the isFormatSupported() function will test if a video surface format is supported.  If a
    format is not supported the nearestFormat() function may be able to suggest a similar format.
    For example, if a surface supports fixed set of resolutions it may suggest the smallest
    supported resolution that contains the proposed resolution.

    The start() function takes a supported format and enables a video surface.  Once started a
    surface will begin displaying the frames it receives in the present() function.  Surfaces may
    hold a reference to the buffer of a presented video frame until a new frame is presented or
    streaming is stopped. The stop() function will disable a surface and a release any video
    buffers it holds references to.
*/

/*!
    \enum QAbstractVideoSurface::Error
    This enum describes the errors that may be returned by the error() function.

    \value NoError No error occurred.
    \value UnsupportedFormatError A video format was not supported.
    \value IncorrectFormatError A video frame was not compatible with the format of the surface.
    \value StoppedError The surface has not been started.
    \value ResourceError The surface could not allocate some resource.
*/

/*!
    Constructs a video surface with the given \a parent.
*/
QAbstractVideoSurface::QAbstractVideoSurface(QObject *parent)
    : QObject(parent)
{
    setProperty("_q_surfaceFormat", QVariant::fromValue(QVideoSurfaceFormat()));
    setProperty("_q_active", false);
    setProperty("_q_error", QVariant::fromValue(QAbstractVideoSurface::NoError));
    setProperty("_q_nativeResolution", QSize());
}

// XXX Qt5
/*!
    \internal

    This is deprecated.

    Since we need to build without access to Qt's private headers we can't reliably inherit
    from QObjectPrivate.  Binary compatibility means we can't remove this constructor or
    add a d pointer to QAbstractVideoSurface.
*/
QAbstractVideoSurface::QAbstractVideoSurface(QAbstractVideoSurfacePrivate &, QObject *parent)
    : QObject(parent)
{
}

/*!
    Destroys a video surface.
*/
QAbstractVideoSurface::~QAbstractVideoSurface()
{
}

/*!
    \fn QAbstractVideoSurface::supportedPixelFormats(QAbstractVideoBuffer::HandleType type) const

    Returns a list of pixel formats a video surface can present for a given handle \a type.

    The pixel formats returned for the QAbstractVideoBuffer::NoHandle type are valid for any buffer
    that can be mapped in read-only mode.

    Types that are first in the list can be assumed to be faster to render.
    \since 1.0
*/

/*!
    Tests a video surface \a format to determine if a surface can accept it.

    Returns true if the format is supported by the surface, and false otherwise.
    \since 1.0
*/
bool QAbstractVideoSurface::isFormatSupported(const QVideoSurfaceFormat &format) const
{
    return supportedPixelFormats(format.handleType()).contains(format.pixelFormat());
}

/*!
    Returns a supported video surface format that is similar to \a format.

    A similar surface format is one that has the same \l {QVideoSurfaceFormat::pixelFormat()}{pixel
    format} and \l {QVideoSurfaceFormat::handleType()}{handle type} but may differ in some of the other
    properties.  For example, if there are restrictions on the \l {QVideoSurfaceFormat::frameSize()}
    {frame sizes} a video surface can accept it may suggest a format with a larger frame size and
    a \l {QVideoSurfaceFormat::viewport()}{viewport} the size of the original frame size.

    If the format is already supported it will be returned unchanged, or if there is no similar
    supported format an invalid format will be returned.
    \since 1.0
*/
QVideoSurfaceFormat QAbstractVideoSurface::nearestFormat(const QVideoSurfaceFormat &format) const
{
    return isFormatSupported(format)
            ? format
            : QVideoSurfaceFormat();
}

/*!
    \fn QAbstractVideoSurface::supportedFormatsChanged()

    Signals that the set of formats supported by a video surface has changed.

    \since 1.0
    \sa supportedPixelFormats(), isFormatSupported()
*/

/*!
    Returns the format of a video surface.
    \since 1.0
*/
QVideoSurfaceFormat QAbstractVideoSurface::surfaceFormat() const
{
    return property("_q_format").value<QVideoSurfaceFormat>();
}

/*!
    \fn QAbstractVideoSurface::surfaceFormatChanged(const QVideoSurfaceFormat &format)

    Signals that the configured \a format of a video surface has changed.

    \since 1.0
    \sa surfaceFormat(), start()
*/

/*!
    Starts a video surface presenting \a format frames.

    Returns true if the surface was started, and false if an error occurred.

    \since 1.0
    \sa isActive(), stop()
*/
bool QAbstractVideoSurface::start(const QVideoSurfaceFormat &format)
{
    bool wasActive  = property("_q_active").toBool();

    setProperty("_q_active", true);
    setProperty("_q_format", QVariant::fromValue(format));
    setProperty("_q_error", QVariant::fromValue(NoError));

    emit surfaceFormatChanged(format);

    if (!wasActive)
        emit activeChanged(true);

    return true;
}

/*!
    Stops a video surface presenting frames and releases any resources acquired in start().

    \since 1.0
    \sa isActive(), start()
*/
void QAbstractVideoSurface::stop()
{
    if (property("_q_active").toBool()) {
        setProperty("_q_format", QVariant::fromValue(QVideoSurfaceFormat()));
        setProperty("_q_active", false);

        emit activeChanged(false);
        emit surfaceFormatChanged(surfaceFormat());
    }
}

/*!
    Indicates whether a video surface has been started.

    Returns true if the surface has been started, and false otherwise.
    \since 1.0
*/
bool QAbstractVideoSurface::isActive() const
{
    return property("_q_active").toBool();
}

/*!
    \fn QAbstractVideoSurface::activeChanged(bool active)

    Signals that the \a active state of a video surface has changed.

    \since 1.0
    \sa isActive(), start(), stop()
*/

/*!
    \fn QAbstractVideoSurface::present(const QVideoFrame &frame)

    Presents a video \a frame.

    Returns true if the frame was presented, and false if an error occurred.

    Not all surfaces will block until the presentation of a frame has completed.  Calling present()
    on a non-blocking surface may fail if called before the presentation of a previous frame has
    completed.  In such cases the surface may not return to a ready state until it has had an
    opportunity to process events.

    If present() fails for any other reason the surface will immediately enter the stopped state
    and an error() value will be set.

    A video surface must be in the started state for present() to succeed, and the format of the
    video frame must be compatible with the current video surface format.

    \since 1.0
    \sa error()
*/

/*!
    Returns the last error that occurred.

    If a surface fails to start(), or stops unexpectedly this function can be called to discover
    what error occurred.
    \since 1.0
*/

QAbstractVideoSurface::Error QAbstractVideoSurface::error() const
{
    return property("_q_error").value<QAbstractVideoSurface::Error>();
}

/*!
    Sets the value of error() to \a error.
    \since 1.0
*/
void QAbstractVideoSurface::setError(Error error)
{
    setProperty("_q_error", QVariant::fromValue(error));
}

/*!
   \property QAbstractVideoSurface::nativeResolution

   The native resolution of video surface.
   This is the resolution of video frames the surface
   can render with optimal quality and/or performance.

   The native resolution is not always known and can be changed during playback.
    \since 1.1
 */
QSize QAbstractVideoSurface::nativeResolution() const
{
    return property("_q_nativeResolution").toSize();
}

/*!
    Set the video surface native \a resolution.
    \since 1.1
 */
void QAbstractVideoSurface::setNativeResolution(const QSize &resolution)
{
    const QSize nativeResolution = property("_q_nativeResolution").toSize();

    if (nativeResolution != resolution) {
        setProperty("_q_nativeResolution", resolution);

        emit nativeResolutionChanged(resolution);
    }
}
/*!
    \fn QAbstractVideoSurface::nativeResolutionChanged(const QSize &resolution);

    Signals the native \a resolution of video surface has changed.
    \since 1.1
*/

QT_END_NAMESPACE

#include "moc_qabstractvideosurface.cpp"

