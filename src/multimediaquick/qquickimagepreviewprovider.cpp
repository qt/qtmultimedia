// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickimagepreviewprovider_p.h"
#include <QtCore/qmutex.h>
#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

struct QQuickImagePreviewProviderPrivate
{
    QString id;
    QImage image;
    QMutex mutex;
};

Q_GLOBAL_STATIC(QQuickImagePreviewProviderPrivate, priv)

QQuickImagePreviewProvider::QQuickImagePreviewProvider()
: QQuickImageProvider(QQuickImageProvider::Image)
{
}

QQuickImagePreviewProvider::~QQuickImagePreviewProvider()
{
    QQuickImagePreviewProviderPrivate *d = priv();
    QMutexLocker lock(&d->mutex);
    d->id.clear();
    d->image = QImage();
}

QImage QQuickImagePreviewProvider::requestImage(const QString &id, QSize *size, const QSize& requestedSize)
{
    QQuickImagePreviewProviderPrivate *d = priv();
    QMutexLocker lock(&d->mutex);

    if (d->id != id)
        return QImage();

    QImage res = d->image;
    if (!requestedSize.isEmpty())
        res = res.scaled(requestedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    if (size)
        *size = res.size();

    return res;
}

void QQuickImagePreviewProvider::registerPreview(const QString &id, const QImage &preview)
{
    //only the last preview is kept
    QQuickImagePreviewProviderPrivate *d = priv();
    QMutexLocker lock(&d->mutex);
    d->id = id;
    d->image = preview;
}

QT_END_NAMESPACE
