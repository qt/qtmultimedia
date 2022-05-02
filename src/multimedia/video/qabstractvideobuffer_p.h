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

#ifndef QABSTRACTVIDEOBUFFER_H
#define QABSTRACTVIDEOBUFFER_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtMultimedia/qtmultimediaglobal.h>
#include <QtMultimedia/qvideoframe.h>

#include <QtCore/qmetatype.h>

QT_BEGIN_NAMESPACE


class QVariant;
class QRhi;

class Q_MULTIMEDIA_EXPORT QAbstractVideoBuffer
{
public:
    QAbstractVideoBuffer(QVideoFrame::HandleType type, QRhi *rhi = nullptr);
    virtual ~QAbstractVideoBuffer();

    QVideoFrame::HandleType handleType() const;

    struct MapData
    {
        int nPlanes = 0;
        int bytesPerLine[4] = {};
        uchar *data[4] = {};
        int size[4] = {};
    };

    virtual QVideoFrame::MapMode mapMode() const = 0;
    virtual MapData map(QVideoFrame::MapMode mode) = 0;
    virtual void unmap() = 0;

    virtual void mapTextures() {}
    virtual quint64 textureHandle(int /*plane*/) const { return 0; }

protected:
    QVideoFrame::HandleType m_type;
    QRhi *rhi = nullptr;

private:
    Q_DISABLE_COPY(QAbstractVideoBuffer)
};

#ifndef QT_NO_DEBUG_STREAM
Q_MULTIMEDIA_EXPORT QDebug operator<<(QDebug, QVideoFrame::MapMode);
#endif

QT_END_NAMESPACE

#endif
