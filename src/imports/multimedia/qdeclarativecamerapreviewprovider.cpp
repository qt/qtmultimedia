/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

#include "qdeclarativecamerapreviewprovider_p.h"
#include <QtCore/qmutex.h>
#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

struct QDeclarativeCameraPreviewProviderPrivate
{
    QString id;
    QImage image;
    QMutex mutex;
};

Q_GLOBAL_STATIC(QDeclarativeCameraPreviewProviderPrivate, qDeclarativeCameraPreviewProviderPrivate)

QDeclarativeCameraPreviewProvider::QDeclarativeCameraPreviewProvider()
: QQuickImageProvider(QQuickImageProvider::Image)
{
}

QDeclarativeCameraPreviewProvider::~QDeclarativeCameraPreviewProvider()
{
    QDeclarativeCameraPreviewProviderPrivate *d = qDeclarativeCameraPreviewProviderPrivate();
    QMutexLocker lock(&d->mutex);
    d->id.clear();
    d->image = QImage();
}

QImage QDeclarativeCameraPreviewProvider::requestImage(const QString &id, QSize *size, const QSize& requestedSize)
{
    QDeclarativeCameraPreviewProviderPrivate *d = qDeclarativeCameraPreviewProviderPrivate();
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

void QDeclarativeCameraPreviewProvider::registerPreview(const QString &id, const QImage &preview)
{
    //only the last preview is kept
    QDeclarativeCameraPreviewProviderPrivate *d = qDeclarativeCameraPreviewProviderPrivate();
    QMutexLocker lock(&d->mutex);
    d->id = id;
    d->image = preview;
}

QT_END_NAMESPACE
