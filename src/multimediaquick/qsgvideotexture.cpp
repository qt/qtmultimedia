// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsgvideotexture_p.h"
#include <QtQuick/qsgtexturematerial.h>
#include <QtQuick/qsgmaterial.h>

QT_BEGIN_NAMESPACE

class QSGVideoTexturePrivate
{
    Q_DECLARE_PUBLIC(QSGVideoTexture)

private:
    QSGVideoTexture *q_ptr = nullptr;
    QRhiTexture::Format m_format;
    QSize m_size;
    QByteArray m_data;
    QRhiTexture *m_texture = nullptr;
};

QSGVideoTexture::QSGVideoTexture()
    : d_ptr(new QSGVideoTexturePrivate)
{
    d_ptr->q_ptr = this;

    // Nearest filtering just looks bad for any text in videos
    setFiltering(Linear);
}

QSGVideoTexture::~QSGVideoTexture() = default;

qint64 QSGVideoTexture::comparisonKey() const
{
    Q_D(const QSGVideoTexture);
    if (d->m_texture)
        return qint64(qintptr(d->m_texture));

    // two textures (and so materials) with not-yet-created texture underneath are never equal
    return qint64(qintptr(this));
}

QRhiTexture *QSGVideoTexture::rhiTexture() const
{
    return d_func()->m_texture;
}

QSize QSGVideoTexture::textureSize() const
{
    return d_func()->m_size;
}

bool QSGVideoTexture::hasAlphaChannel() const
{
    Q_D(const QSGVideoTexture);
    return d->m_format == QRhiTexture::RGBA8 || d->m_format == QRhiTexture::BGRA8;
}

bool QSGVideoTexture::hasMipmaps() const
{
    return mipmapFiltering() != QSGTexture::None;
}

void QSGVideoTexture::setData(QRhiTexture::Format f, const QSize &s, const uchar *data, int bytes)
{
    Q_D(QSGVideoTexture);
    d->m_size = s;
    d->m_format = f;
    d->m_data = {reinterpret_cast<const char *>(data), bytes};
}

void QSGVideoTexture::setRhiTexture(QRhiTexture *texture)
{
    d_func()->m_texture = texture;
}

QT_END_NAMESPACE
