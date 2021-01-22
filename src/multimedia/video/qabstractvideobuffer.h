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

#ifndef QABSTRACTVIDEOBUFFER_H
#define QABSTRACTVIDEOBUFFER_H

#include <QtMultimedia/qtmultimediaglobal.h>
#include <QtMultimedia/qmultimedia.h>


#include <QtCore/qmetatype.h>

QT_BEGIN_NAMESPACE


class QVariant;

class QAbstractVideoBufferPrivate;

class Q_MULTIMEDIA_EXPORT QAbstractVideoBuffer
{
public:
    enum HandleType
    {
        NoHandle,
        GLTextureHandle,
        XvShmImageHandle,
        CoreImageHandle,
        QPixmapHandle,
        EGLImageHandle,
        UserHandle = 1000
    };

    enum MapMode
    {
        NotMapped = 0x00,
        ReadOnly  = 0x01,
        WriteOnly = 0x02,
        ReadWrite = ReadOnly | WriteOnly
    };

    QAbstractVideoBuffer(HandleType type);
    virtual ~QAbstractVideoBuffer();
    virtual void release();

    HandleType handleType() const;

    virtual MapMode mapMode() const = 0;

    virtual uchar *map(MapMode mode, int *numBytes, int *bytesPerLine) = 0;
    int mapPlanes(MapMode mode, int *numBytes, int bytesPerLine[4], uchar *data[4]);
    virtual void unmap() = 0;

    virtual QVariant handle() const;

protected:
    QAbstractVideoBuffer(QAbstractVideoBufferPrivate &dd, HandleType type);

    QAbstractVideoBufferPrivate *d_ptr;  // For expansion, not used currently
    HandleType m_type;

private:
    Q_DECLARE_PRIVATE(QAbstractVideoBuffer)
    Q_DISABLE_COPY(QAbstractVideoBuffer)
};

class QAbstractPlanarVideoBufferPrivate;
class Q_MULTIMEDIA_EXPORT QAbstractPlanarVideoBuffer : public QAbstractVideoBuffer
{
public:
    QAbstractPlanarVideoBuffer(HandleType type);
    virtual ~QAbstractPlanarVideoBuffer();

    uchar *map(MapMode mode, int *numBytes, int *bytesPerLine) override;
    virtual int map(MapMode mode, int *numBytes, int bytesPerLine[4], uchar *data[4]) = 0;

protected:
    QAbstractPlanarVideoBuffer(QAbstractPlanarVideoBufferPrivate &dd, HandleType type);

private:
    Q_DISABLE_COPY(QAbstractPlanarVideoBuffer)
};

#ifndef QT_NO_DEBUG_STREAM
Q_MULTIMEDIA_EXPORT QDebug operator<<(QDebug, QAbstractVideoBuffer::HandleType);
Q_MULTIMEDIA_EXPORT QDebug operator<<(QDebug, QAbstractVideoBuffer::MapMode);
#endif

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QAbstractVideoBuffer::HandleType)
Q_DECLARE_METATYPE(QAbstractVideoBuffer::MapMode)

#endif
