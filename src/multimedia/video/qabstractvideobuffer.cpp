// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qabstractvideobuffer.h"

QT_BEGIN_NAMESPACE

/*!
    \class QAbstractVideoBuffer
    \since 6.8
    \brief The QAbstractVideoBuffer class is an abstraction for video data.
    \inmodule QtMultimedia
    \ingroup multimedia
    \ingroup multimedia_video

    The \l QVideoFrame class makes use of a QAbstractVideoBuffer internally to reference a buffer of
    video data. Creating a subclass of QAbstractVideoBuffer allows you to construct video
    frames from preallocated or static buffers. The subclass can contain a hardware buffer,
    and implement access to the data by mapping the buffer to CPU memory.

    The contents of a buffer can be accessed by mapping the buffer to memory using the map()
    function, which returns a structure containing information about plane layout of the current
    video data.

    \sa QVideoFrame, QVideoFrameFormat, QVideoFrame::MapMode
*/

/*!
    \class QAbstractVideoBuffer::MapData
    \brief The QAbstractVideoBuffer::MapData structure describes the mapped plane layout.
    \inmodule QtMultimedia
    \ingroup multimedia
    \ingroup multimedia_video

    The structure contains a number of mapped planes, and plane data for each plane,
    specificly, a number of bytes per line, a data pointer, and a data size.
    The structure doesn't hold any ownership of the data it refers to.

    A defaultly created structure means that no data has been mapped.

    All the values in the structure default to zeros.

    \sa QAbstractVideoBuffer::map
*/

/*!
    \variable QAbstractVideoBuffer::MapData::planeCount

    The number of planes of the mapped video data. If the format of the data
    is multiplanar, and the value is \c 1, the actual plane layout will
    be calculated upon invoking of \l QVideoFrame::map from the frame height,
    \c{bytesPerLine[0]}, and \c{dataSize[0]}.

    Defaults to \c 0.
*/

/*!
    \variable QAbstractVideoBuffer::MapData::bytesPerLine

    The array of numbrers of bytes per line for each
    plane from \c 0 to \c{planeCount - 1}.

    The values of the array default to \c 0.
*/

/*!
    \variable QAbstractVideoBuffer::MapData::data

    The array of pointers to the mapped video pixel data
    for each plane from \c 0 to \c{planeCount - 1}.
    The implementation of QAbstractVideoBuffer must hold ownership of the data
    at least until \l QAbstractVideoBuffer::unmap is called.

    The values of the array default to \c nullptr.
*/

/*!
    \variable QAbstractVideoBuffer::MapData::dataSize

    The array of sizes in bytes of the mapped video pixel data
    for each plane from \c 0 to \c{planeCount - 1}.

    The values of the array default to \c 0.
*/

// must be out-of-line to ensure correct working of dynamic_cast when QHwVideoBuffer is created in tests
/*!
    Destroys a video buffer.
*/
QAbstractVideoBuffer::~QAbstractVideoBuffer() = default;

/*! \fn QAbstractVideoBuffer::MapData QAbstractVideoBuffer::map(QVideoFrame::MapMode mode)

    Maps the planes of a video buffer to memory.

    Returns a \l MapData structure that contains information about the plane layout of
    the mapped current video data. If the mapping fails, the method returns the default structure.
    For CPU memory buffers, the data is considered as already mapped, so the function
    just returns the plane layout of the preallocated underlying data.

    The map \a mode indicates whether the contents of the mapped memory should be read from and/or
    written to the buffer.  If the map mode includes the \c QVideoFrame::ReadOnly flag the
    mapped memory will be populated with the content of the buffer when initially mapped.  If the map
    mode includes the \c QVideoFrame::WriteOnly flag the content of the possibly modified
    mapped memory will be written back to the buffer when unmapped.

    When access to the data is no longer needed, the \l unmap function is called
    to release the mapped memory and possibly update the buffer contents.

    If the format of the video data is multiplanar, the method may map the whole pixel data
    as a single plane. In this case, mapping a buffer with \l QVideoFrame
    will calculate additional planes from the specified line stride of the first plane,
    the frame height, and the data size.
*/

/*!
    \fn QAbstractVideoBuffer::unmap()

    Releases the memory mapped by the map() function.

    If the \l {QVideoFrame::MapMode}{MapMode} included the \c QVideoFrame::WriteOnly
    flag this will write the current content of the mapped memory back to the video frame.

    For CPU video buffers, the function may be not overridden.
    The default implementation of \c unmap does nothing.

    \sa map()
*/
void QAbstractVideoBuffer::unmap() { }

/*!
    \fn QAbstractVideoBuffer::format() const

    Gets \l QVideoFrameFormat of the underlying video buffer.

    The format must be available upon construction of \l QVideoFrame.
    QVideoFrame will contain won instance of the given format, that
    can be detached and modified.
*/

QT_END_NAMESPACE
