// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsgvideotexture_p.h"
#include <QtQuick/qsgtexturematerial.h>
#include <QtQuick/qsgmaterial.h>

QT_BEGIN_NAMESPACE

class QSGVideoTexturePrivate
{
    Q_DECLARE_PUBLIC(QSGVideoTexture)
public:
    void updateRhiTexture(QRhi *rhi, QRhiResourceUpdateBatch *resourceUpdates);

private:
    QSGVideoTexture *q_ptr = nullptr;
    QRhiTexture::Format m_format;
    QSize m_size;
    QByteArray m_data;

    std::unique_ptr<QRhiTexture> m_texture;
    quint64 m_nativeObject = 0;
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
    if (d->m_nativeObject)
        return d->m_nativeObject;

    if (d->m_texture)
        return qint64(qintptr(d->m_texture.get()));

    // two textures (and so materials) with not-yet-created texture underneath are never equal
    return qint64(qintptr(this));
}

QRhiTexture *QSGVideoTexture::rhiTexture() const
{
    return d_func()->m_texture.get();
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

void QSGVideoTexture::setNativeObject(quint64 obj, const QSize &s, QRhiTexture::Format f)
{
    Q_D(QSGVideoTexture);
    setData(f, s, nullptr, 0);
    if (d->m_nativeObject != obj) {
        d->m_nativeObject = obj;
        d->m_texture.reset();
    }
}

void QSGVideoTexturePrivate::updateRhiTexture(QRhi *rhi, QRhiResourceUpdateBatch *resourceUpdates)
{
    Q_Q(QSGVideoTexture);

    bool needsRebuild = m_texture && m_texture->pixelSize() != m_size;
    if (!m_texture) {
        QRhiTexture::Flags flags;
        if (q->hasMipmaps())
            flags |= QRhiTexture::MipMapped | QRhiTexture::UsedWithGenerateMips;

        m_texture.reset(rhi->newTexture(m_format, m_size, 1, flags));
        needsRebuild = true;
    }

    if (needsRebuild) {
        m_texture->setPixelSize(m_size);
        bool created = m_nativeObject
            ? m_texture->createFrom({m_nativeObject, 0})
            : m_texture->create();
        if (!created) {
            qWarning("Failed to build texture (size %dx%d)",
                m_size.width(), m_size.height());
            return;
        }
    }

    if (!m_data.isEmpty()) {
        QRhiTextureSubresourceUploadDescription subresDesc(m_data.constData(), m_data.size());
        subresDesc.setSourceSize(m_size);
        subresDesc.setDestinationTopLeft(QPoint(0, 0));
        QRhiTextureUploadEntry entry(0, 0, subresDesc);
        QRhiTextureUploadDescription desc({ entry });
        resourceUpdates->uploadTexture(m_texture.get(), desc);
        m_data.clear();
    }
}

void QSGVideoTexture::commitTextureOperations(QRhi *rhi, QRhiResourceUpdateBatch *resourceUpdates)
{
    d_func()->updateRhiTexture(rhi, resourceUpdates);
}

QRhiTexture *QSGVideoTexture::releaseTexture()
{
    return d_func()->m_texture.release();
}

void QSGVideoTexture::setRhiTexture(QRhiTexture *texture)
{
    d_func()->m_texture.reset(texture);
}

QT_END_NAMESPACE
