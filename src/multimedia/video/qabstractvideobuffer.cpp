/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qabstractvideobuffer_p.h"

#include <qvariant.h>

#include <QDebug>


QT_BEGIN_NAMESPACE

static void qRegisterAbstractVideoBufferMetaTypes()
{
    qRegisterMetaType<QAbstractVideoBuffer::HandleType>();
    qRegisterMetaType<QAbstractVideoBuffer::MapMode>();
}

Q_CONSTRUCTOR_FUNCTION(qRegisterAbstractVideoBufferMetaTypes)


/*!
    \class QAbstractVideoBuffer
    \brief The QAbstractVideoBuffer class is an abstraction for video data.
    \inmodule QtMultimedia
    \ingroup multimedia
    \ingroup multimedia_video

    The QVideoFrame class makes use of a QAbstractVideoBuffer internally to reference a buffer of
    video data.  Quite often video data buffers may reside in video memory rather than system
    memory, and this class provides an abstraction of the location.

    In addition, creating a subclass of QAbstractVideoBuffer will allow you to construct video
    frames from preallocated or static buffers, in cases where the QVideoFrame constructors
    taking a QByteArray or a QImage do not suffice.  This may be necessary when implementing
    a new hardware accelerated video system, for example.

    The contents of a buffer can be accessed by mapping the buffer to memory using the map()
    function, which returns a pointer to memory containing the contents of the video buffer.
    The memory returned by map() is released by calling the unmap() function.

    The handle() of a buffer may also be used to manipulate its contents using type specific APIs.
    The type of a buffer's handle is given by the handleType() function.

    \sa QVideoFrame
*/

/*!
    \enum QAbstractVideoBuffer::HandleType

    Identifies the type of a video buffers handle.

    \value NoHandle The buffer has no handle, its data can only be accessed by mapping the buffer.
    \value GLTextureHandle The handle of the buffer is an OpenGL texture ID.
    \value XvShmImageHandle The handle contains pointer to shared memory XVideo image.
    \value CoreImageHandle The handle contains pointer to Mac OS X CIImage.
    \value QPixmapHandle The handle of the buffer is a QPixmap.
    \value UserHandle Start value for user defined handle types.

    \sa handleType()
*/

/*!
    \enum QAbstractVideoBuffer::MapMode

    Enumerates how a video buffer's data is mapped to system memory.

    \value NotMapped The video buffer is not mapped to memory.
    \value ReadOnly The mapped memory is populated with data from the video buffer when mapped, but
    the content of the mapped memory may be discarded when unmapped.
    \value WriteOnly The mapped memory is uninitialized when mapped, but the possibly modified content
    will be used to populate the video buffer when unmapped.
    \value ReadWrite The mapped memory is populated with data from the video buffer, and the
    video buffer is repopulated with the content of the mapped memory when it is unmapped.

    \sa mapMode(), map()
*/

/*!
    Constructs an abstract video buffer of the given \a type.
*/
QAbstractVideoBuffer::QAbstractVideoBuffer(HandleType type)
    : d_ptr(0)
    , m_type(type)
{
}

/*!
    \internal
*/
QAbstractVideoBuffer::QAbstractVideoBuffer(QAbstractVideoBufferPrivate &dd, HandleType type)
    : d_ptr(&dd)
    , m_type(type)
{
}

/*!
    Destroys an abstract video buffer.
*/
QAbstractVideoBuffer::~QAbstractVideoBuffer()
{
    delete d_ptr;
}

/*!
    Releases the video buffer.

    QVideoFrame calls QAbstractVideoBuffer::release when the buffer is not used
    any more and can be destroyed or returned to the buffer pool.

    The default implementation deletes the buffer instance.
*/
void QAbstractVideoBuffer::release()
{
    delete this;
}

/*!
    Returns the type of a video buffer's handle.

    \sa handle()
*/
QAbstractVideoBuffer::HandleType QAbstractVideoBuffer::handleType() const
{
    return m_type;
}

/*!
    \fn QAbstractVideoBuffer::mapMode() const

    Returns the mode a video buffer is mapped in.

    \sa map()
*/

/*!
    \fn QAbstractVideoBuffer::map(MapMode mode, int *numBytes, int *bytesPerLine)

    Maps the contents of a video buffer to memory.

    In some cases the video buffer might be stored in video memory or otherwise inaccessible
    memory, so it is necessary to map the buffer before accessing the pixel data.  This may involve
    copying the contents around, so avoid mapping and unmapping unless required.

    The map \a mode indicates whether the contents of the mapped memory should be read from and/or
    written to the buffer.  If the map mode includes the \c QAbstractVideoBuffer::ReadOnly flag the
    mapped memory will be populated with the content of the buffer when initially mapped.  If the map
    mode includes the \c QAbstractVideoBuffer::WriteOnly flag the content of the possibly modified
    mapped memory will be written back to the buffer when unmapped.

    When access to the data is no longer needed be sure to call the unmap() function to release the
    mapped memory and possibly update the buffer contents.

    Returns a pointer to the mapped memory region, or a null pointer if the mapping failed.  The
    size in bytes of the mapped memory region is returned in \a numBytes, and the line stride in \a
    bytesPerLine.

    \note Writing to memory that is mapped as read-only is undefined, and may result in changes
    to shared data or crashes.

    \sa unmap(), mapMode()
*/

/*!
    \fn QAbstractVideoBuffer::unmap()

    Releases the memory mapped by the map() function.

    If the \l {QAbstractVideoBuffer::MapMode}{MapMode} included the \c QAbstractVideoBuffer::WriteOnly
    flag this will write the current content of the mapped memory back to the video frame.

    \sa map()
*/

/*!
    Returns a type specific handle to the data buffer.

    The type of the handle is given by handleType() function.

    \sa handleType()
*/
QVariant QAbstractVideoBuffer::handle() const
{
    return QVariant();
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, QAbstractVideoBuffer::HandleType type)
{
    switch (type) {
    case QAbstractVideoBuffer::NoHandle:
        return dbg.nospace() << "NoHandle";
    case QAbstractVideoBuffer::GLTextureHandle:
        return dbg.nospace() << "GLTextureHandle";
    case QAbstractVideoBuffer::XvShmImageHandle:
        return dbg.nospace() << "XvShmImageHandle";
    case QAbstractVideoBuffer::CoreImageHandle:
        return dbg.nospace() << "CoreImageHandle";
    case QAbstractVideoBuffer::QPixmapHandle:
        return dbg.nospace() << "QPixmapHandle";
    default:
        return dbg.nospace() << QString(QLatin1String("UserHandle(%1)")).arg(int(type)).toLatin1().constData();
    }
}

QDebug operator<<(QDebug dbg, QAbstractVideoBuffer::MapMode mode)
{
    switch (mode) {
    case QAbstractVideoBuffer::ReadOnly:
        return dbg.nospace() << "ReadOnly";
    case QAbstractVideoBuffer::ReadWrite:
        return dbg.nospace() << "ReadWrite";
    case QAbstractVideoBuffer::WriteOnly:
        return dbg.nospace() << "WriteOnly";
    default:
        return dbg.nospace() << "NotMapped";
    }
}
#endif

QT_END_NAMESPACE
