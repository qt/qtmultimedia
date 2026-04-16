// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qgstreamervideosource.h"
#include "qgstreamervideosource_p.h"

#include <QtMultimedia/private/qplatformcamera_p.h>
#include <QtMultimedia/private/qplatformmediaintegration_p.h>

#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

QGStreamerVideoSource::QGStreamerVideoSource(const QString &gstBinDescription, QObject *parent)
    : QObject(*new QGStreamerVideoSourcePrivate, parent)
{
    Q_D(QGStreamerVideoSource);

    auto maybeControl = QPlatformMediaIntegration::instance()->createGStreamerVideoSource(
            this, gstBinDescription);
    if (!maybeControl) {
        qWarning() << "Failed to initialize QGStreamerVideoSource" << maybeControl.error();
        return;
    }

    d->gstBinDescription = gstBinDescription;
    d->control = *maybeControl;

    connect(d->control, &QPlatformVideoSource::activeChanged, this,
            &QGStreamerVideoSource::activeChanged);
}

QGStreamerVideoSource::~QGStreamerVideoSource() = default;

bool QGStreamerVideoSource::isActive() const
{
    Q_D(const QGStreamerVideoSource);
    return d->control && d->control->isActive();
}

QString QGStreamerVideoSource::gstBinDescription() const
{
    Q_D(const QGStreamerVideoSource);
    return d->gstBinDescription;
}

void QGStreamerVideoSource::setActive(bool active)
{
    Q_D(const QGStreamerVideoSource);
    if (d->control)
        d->control->setActive(active);
}

QPlatformCamera *QGStreamerVideoSource::platformVideoSource() const
{
    Q_D(const QGStreamerVideoSource);
    return d->control;
}

QT_END_NAMESPACE
