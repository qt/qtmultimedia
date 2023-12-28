// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
#include <QtGui/qmatrix4x4.h>
#include <QtCore/private/qglobal_p.h>

#include <memory>

QT_BEGIN_NAMESPACE


class QVariant;
class QRhi;
class QRhiTexture;

class Q_MULTIMEDIA_EXPORT QVideoFrameTextures
{
public:
    virtual ~QVideoFrameTextures() {}
    virtual QRhiTexture *texture(uint plane) const = 0;
};

class Q_MULTIMEDIA_EXPORT QAbstractVideoBuffer
{
public:
    QAbstractVideoBuffer(QVideoFrame::HandleType type, QRhi *rhi = nullptr);
    virtual ~QAbstractVideoBuffer();

    QVideoFrame::HandleType handleType() const;
    QRhi *rhi() const;

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

    virtual std::unique_ptr<QVideoFrameTextures> mapTextures(QRhi *) { return {}; }
    virtual quint64 textureHandle(int /*plane*/) const { return 0; }

    virtual QMatrix4x4 externalTextureMatrix() const { return {}; }

    virtual QByteArray underlyingByteArray(int /*plane*/) const { return {}; }
protected:
    QVideoFrame::HandleType m_type;
    QRhi *m_rhi = nullptr;

private:
    Q_DISABLE_COPY(QAbstractVideoBuffer)
};

#ifndef QT_NO_DEBUG_STREAM
Q_MULTIMEDIA_EXPORT QDebug operator<<(QDebug, QVideoFrame::MapMode);
#endif

QT_END_NAMESPACE

#endif
