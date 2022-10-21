/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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

    QScopedPointer<QRhiTexture> m_texture;
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
        return qint64(qintptr(d->m_texture.data()));

    // two textures (and so materials) with not-yet-created texture underneath are never equal
    return qint64(qintptr(this));
}

QRhiTexture *QSGVideoTexture::rhiTexture() const
{
    return d_func()->m_texture.data();
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
        resourceUpdates->uploadTexture(m_texture.data(), desc);
        m_data.clear();
    }
}

void QSGVideoTexture::commitTextureOperations(QRhi *rhi, QRhiResourceUpdateBatch *resourceUpdates)
{
    d_func()->updateRhiTexture(rhi, resourceUpdates);
}

void QSGVideoTexture::setRhiTexture(QRhiTexture *texture)
{
    d_func()->m_texture.reset(texture);
}

QT_END_NAMESPACE
