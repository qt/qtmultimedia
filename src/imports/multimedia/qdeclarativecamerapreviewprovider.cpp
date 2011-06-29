/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
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
: QDeclarativeImageProvider(QDeclarativeImageProvider::Image)
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
