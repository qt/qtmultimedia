// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qmemoryvideobuffer_p.h"

QT_BEGIN_NAMESPACE

/*!
    \class QMemoryVideoBuffer
    \brief The QMemoryVideoBuffer class provides a system memory allocated video data buffer.
    \internal

    QMemoryVideoBuffer is the default video buffer for allocating system memory.  It may be used to
    allocate memory for a QVideoFrame without implementing your own QAbstractVideoBuffer.
*/

/*!
    Constructs a video buffer with an image stride of \a bytesPerLine from a byte \a array.
*/
QMemoryVideoBuffer::QMemoryVideoBuffer(const QByteArray &array, int bytesPerLine)
    : QAbstractVideoBuffer(QVideoFrame::NoHandle)
{
    data = array;
    this->bytesPerLine = bytesPerLine;
}

/*!
    Destroys a system memory allocated video buffer.
*/
QMemoryVideoBuffer::~QMemoryVideoBuffer() = default;

/*!
    \reimp
*/
QVideoFrame::MapMode QMemoryVideoBuffer::mapMode() const
{
    return m_mapMode;
}

/*!
    \reimp
*/
QAbstractVideoBuffer::MapData QMemoryVideoBuffer::map(QVideoFrame::MapMode mode)
{
    MapData mapData;
    if (m_mapMode == QVideoFrame::NotMapped && data.size() && mode != QVideoFrame::NotMapped) {
        m_mapMode = mode;

        mapData.nPlanes = 1;
        mapData.bytesPerLine[0] = bytesPerLine;
        mapData.data[0] = reinterpret_cast<uchar *>(data.data());
        mapData.size[0] = data.size();
    }

    return mapData;
}

/*!
    \reimp
*/
void QMemoryVideoBuffer::unmap()
{
    m_mapMode = QVideoFrame::NotMapped;
}

QT_END_NAMESPACE
