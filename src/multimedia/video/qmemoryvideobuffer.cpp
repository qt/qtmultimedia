/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
****************************************************************************/

#include "qmemoryvideobuffer_p.h"

#include "qabstractvideobuffer_p.h"
#include <qbytearray.h>

QT_BEGIN_NAMESPACE

class QMemoryVideoBufferPrivate : public QAbstractVideoBufferPrivate
{
public:
    QMemoryVideoBufferPrivate()
        : bytesPerLine(0)
        , mapMode(QAbstractVideoBuffer::NotMapped)
    {
    }

    int bytesPerLine;
    QAbstractVideoBuffer::MapMode mapMode;
    QByteArray data;
};

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
    : QAbstractVideoBuffer(*new QMemoryVideoBufferPrivate, NoHandle)
{
    Q_D(QMemoryVideoBuffer);

    d->data = array;
    d->bytesPerLine = bytesPerLine;
}

/*!
    Destroys a system memory allocated video buffer.
*/
QMemoryVideoBuffer::~QMemoryVideoBuffer()
{
}

/*!
    \reimp
*/
QAbstractVideoBuffer::MapMode QMemoryVideoBuffer::mapMode() const
{
    return d_func()->mapMode;
}

/*!
    \reimp
*/
uchar *QMemoryVideoBuffer::map(MapMode mode, int *numBytes, int *bytesPerLine)
{
    Q_D(QMemoryVideoBuffer);

    if (d->mapMode == NotMapped && d->data.size() && mode != NotMapped) {
        d->mapMode = mode;

        if (numBytes)
            *numBytes = d->data.size();

        if (bytesPerLine)
            *bytesPerLine = d->bytesPerLine;

        return reinterpret_cast<uchar *>(d->data.data());
    } else {
        return nullptr;
    }
}

/*!
    \reimp
*/
void QMemoryVideoBuffer::unmap()
{
    d_func()->mapMode = NotMapped;
}

QT_END_NAMESPACE
