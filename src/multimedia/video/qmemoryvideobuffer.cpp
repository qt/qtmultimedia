/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

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
