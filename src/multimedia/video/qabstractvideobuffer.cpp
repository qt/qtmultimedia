// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qabstractvideobuffer_p.h"

#include <qvariant.h>
#include <rhi/qrhi.h>

#include <QDebug>


QT_BEGIN_NAMESPACE

/*!
    \class QAbstractVideoBuffer
    \internal
    \brief The QAbstractVideoBuffer class is an abstraction for video data.
    \inmodule QtMultimedia
    \ingroup multimedia
    \ingroup multimedia_video

    The QVideoFrame class makes use of a QAbstractVideoBuffer internally to reference a buffer of
    video data.  Quite often video data buffers may reside in video memory rather than system
    memory, and this class provides an abstraction of the location.

    In addition, creating a subclass of QAbstractVideoBuffer will allow you to construct video
    frames from preallocated or static buffers. This caters for cases where the QVideoFrame constructors
    taking a QByteArray or a QImage do not suffice. This may be necessary when implementing
    a new hardware accelerated video system, for example.

    The contents of a buffer can be accessed by mapping the buffer to memory using the map()
    function, which returns a pointer to memory containing the contents of the video buffer.
    The memory returned by map() is released by calling the unmap() function.

    The handle() of a buffer may also be used to manipulate its contents using type specific APIs.
    The type of a buffer's handle is given by the handleType() function.

    \sa QVideoFrame
*/

/*!
    \enum QVideoFrame::HandleType

    Identifies the type of a video buffers handle.

    \value NoHandle
    The buffer has no handle, its data can only be accessed by mapping the buffer.
    \value RhiTextureHandle
    The handle of the buffer is defined by The Qt Rendering Hardware Interface
    (RHI). RHI is Qt's internal graphics abstraction for 3D APIs, such as
    OpenGL, Vulkan, Metal, and Direct 3D.

    \sa handleType()
*/

/*!
    \enum QVideoFrame::MapMode

    Enumerates how a video buffer's data is mapped to system memory.

    \value NotMapped
    The video buffer is not mapped to memory.
    \value ReadOnly
    The mapped memory is populated with data from the video buffer when mapped,
    but the content of the mapped memory may be discarded when unmapped.
    \value WriteOnly
    The mapped memory is uninitialized when mapped, but the possibly modified
    content will be used to populate the video buffer when unmapped.
    \value ReadWrite
    The mapped memory is populated with data from the video
    buffer, and the video buffer is repopulated with the content of the mapped
    memory when it is unmapped.

    \sa mapMode(), map()
*/

/*!
    Constructs an abstract video buffer of the given \a type.
*/
QAbstractVideoBuffer::QAbstractVideoBuffer(QVideoFrame::HandleType type, QRhi *rhi)
    : m_type(type),
      m_rhi(rhi)
{
}

/*!
    Destroys an abstract video buffer.
*/
QAbstractVideoBuffer::~QAbstractVideoBuffer()
{
}

/*!
    Returns the type of a video buffer's handle.

    \sa handle()
*/
QVideoFrame::HandleType QAbstractVideoBuffer::handleType() const
{
    return m_type;
}

/*!
    Returns the QRhi instance.
*/
QRhi *QAbstractVideoBuffer::rhi() const
{
    return m_rhi;
}

/*! \fn uchar *QAbstractVideoBuffer::map(MapMode mode, int *numBytes, int *bytesPerLine)

    Independently maps the planes of a video buffer to memory.

    The map \a mode indicates whether the contents of the mapped memory should be read from and/or
    written to the buffer.  If the map mode includes the \c QVideoFrame::ReadOnly flag the
    mapped memory will be populated with the content of the buffer when initially mapped.  If the map
    mode includes the \c QVideoFrame::WriteOnly flag the content of the possibly modified
    mapped memory will be written back to the buffer when unmapped.

    When access to the data is no longer needed be sure to call the unmap() function to release the
    mapped memory and possibly update the buffer contents.

    Returns the number of planes in the mapped video data.  For each plane the line stride of that
    plane will be returned in \a bytesPerLine, and a pointer to the plane data will be returned in
    \a data.  The accumulative size of the mapped data is returned in \a numBytes.

    Not all buffer implementations will map more than the first plane, if this returns a single
    plane for a planar format the additional planes will have to be calculated from the line stride
    of the first plane and the frame height.  Mapping a buffer with QVideoFrame will do this for
    you.

    To implement this function create a derivative of QAbstractPlanarVideoBuffer and implement
    its map function instance instead.

    \since 5.4
*/

/*!
    \fn QAbstractVideoBuffer::unmap()

    Releases the memory mapped by the map() function.

    If the \l {QVideoFrame::MapMode}{MapMode} included the \c QVideoFrame::WriteOnly
    flag this will write the current content of the mapped memory back to the video frame.

    \sa map()
*/

/*! \fn quint64 QAbstractVideoBuffer::textureHandle(QRhi *rhi, int plane) const

    Returns a texture handle to the data buffer.

    \sa handleType()
*/

/*
    \fn int QAbstractPlanarVideoBuffer::map(MapMode mode, int *numBytes, int bytesPerLine[4], uchar *data[4])

    Maps the contents of a video buffer to memory.

    The map \a mode indicates whether the contents of the mapped memory should be read from and/or
    written to the buffer.  If the map mode includes the \c QVideoFrame::ReadOnly flag the
    mapped memory will be populated with the content of the buffer when initially mapped.  If the map
    mode includes the \c QVideoFrame::WriteOnly flag the content of the possibly modified
    mapped memory will be written back to the buffer when unmapped.

    When access to the data is no longer needed be sure to call the unmap() function to release the
    mapped memory and possibly update the buffer contents.

    Returns the number of planes in the mapped video data.  For each plane the line stride of that
    plane will be returned in \a bytesPerLine, and a pointer to the plane data will be returned in
    \a data.  The accumulative size of the mapped data is returned in \a numBytes.

    \sa QAbstractVideoBuffer::map(), QAbstractVideoBuffer::unmap(), QVideoFrame::mapMode()
*/

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, QVideoFrame::MapMode mode)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace();
    switch (mode) {
    case QVideoFrame::ReadOnly:
        return dbg << "ReadOnly";
    case QVideoFrame::ReadWrite:
        return dbg << "ReadWrite";
    case QVideoFrame::WriteOnly:
        return dbg << "WriteOnly";
    default:
        return dbg << "NotMapped";
    }
}
#endif

QT_END_NAMESPACE
