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

#include "qimagevideobuffer_p.h"

#include "qabstractvideobuffer_p.h"

#include <qimage.h>
#include <qvariant.h>

QT_BEGIN_NAMESPACE

/*!
 * \class QImageVideoBuffer
 * \internal
 *
 * A video buffer class for a QImage.
 */
class QImageVideoBufferPrivate : public QAbstractVideoBufferPrivate
{
public:
    QImageVideoBufferPrivate()
        : mapMode(QAbstractVideoBuffer::NotMapped)
    {
    }

    QAbstractVideoBuffer::MapMode mapMode;
    QImage image;
};

QImageVideoBuffer::QImageVideoBuffer(const QImage &image)
    : QAbstractVideoBuffer(*new QImageVideoBufferPrivate, NoHandle)
{
    Q_D(QImageVideoBuffer);

    d->image = image;
}

QImageVideoBuffer::~QImageVideoBuffer()
{
}

QAbstractVideoBuffer::MapMode QImageVideoBuffer::mapMode() const
{
    return d_func()->mapMode;
}

uchar *QImageVideoBuffer::map(MapMode mode, int *numBytes, int *bytesPerLine)
{
    Q_D(QImageVideoBuffer);

    if (d->mapMode == NotMapped && d->image.bits() && mode != NotMapped) {
        d->mapMode = mode;

        if (numBytes)
            *numBytes = int(d->image.sizeInBytes());

        if (bytesPerLine)
            *bytesPerLine = d->image.bytesPerLine();

        return d->image.bits();
    } else {
        return nullptr;
    }
}

void QImageVideoBuffer::unmap()
{
    Q_D(QImageVideoBuffer);

    d->mapMode = NotMapped;
}

QT_END_NAMESPACE
