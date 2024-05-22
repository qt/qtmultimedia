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
QMemoryVideoBuffer::QMemoryVideoBuffer(QByteArray data, int bytesPerLine)
    : QAbstractVideoBuffer(QVideoFrame::NoHandle),
      m_bytesPerLine(bytesPerLine),
      m_data(std::move(data))
{
}

/*!
    Destroys a system memory allocated video buffer.
*/
QMemoryVideoBuffer::~QMemoryVideoBuffer() = default;

/*!
    \reimp
*/
QAbstractVideoBuffer::MapData QMemoryVideoBuffer::map(QVideoFrame::MapMode mode)
{
    MapData mapData;
    if (m_mapMode == QVideoFrame::NotMapped && m_data.size() && mode != QVideoFrame::NotMapped) {
        m_mapMode = mode;

        mapData.nPlanes = 1;
        mapData.bytesPerLine[0] = m_bytesPerLine;
        // avoid detaching and extra copying in case the underlyingByteArray is
        // being held by textures or anything else.
        if (mode == QVideoFrame::ReadOnly)
            mapData.data[0] = reinterpret_cast<uchar *>(const_cast<char*>(m_data.constData()));
        else
            mapData.data[0] = reinterpret_cast<uchar *>(m_data.data());
        mapData.size[0] = m_data.size();
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
