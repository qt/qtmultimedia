// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qgstreamervideosource.h"
#include "qgstreamervideosource_p.h"

#include <QtMultimedia/private/qplatformcamera_p.h>
#include <QtMultimedia/private/qplatformmediaintegration_p.h>

#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

void QGStreamerVideoSourcePrivate::createPlatformCamera(QGStreamerVideoSource *source,
                                                        GstElementOrDescription elementOrDesc)
{
    Q_ASSERT(!platformCamera);

    auto maybePlatformCamera = QPlatformMediaIntegration::instance()->createGStreamerVideoSource(
            source, elementOrDesc);
    if (!maybePlatformCamera) {
        qWarning() << "Failed to initialize QGStreamerVideoSource" << maybePlatformCamera.error();
        return;
    }

    if (auto gstBinDesc = std::get_if<QString>(&elementOrDesc))
        gstBinDescription = std::move(*gstBinDesc);

    platformCamera = *maybePlatformCamera;

    QObject::connect(platformCamera, &QPlatformVideoSource::activeChanged, source,
                     &QGStreamerVideoSource::activeChanged);
}

QGStreamerVideoSource::QGStreamerVideoSource(const QString &gstBinDescription, QObject *parent)
    : QObject(*new QGStreamerVideoSourcePrivate, parent)
{
    Q_D(QGStreamerVideoSource);
    d->createPlatformCamera(this, gstBinDescription);
}

QGStreamerVideoSource::QGStreamerVideoSource(GstElement *gstElement, QObject *parent)
    : QObject(*new QGStreamerVideoSourcePrivate, parent)
{
    Q_D(QGStreamerVideoSource);
    d->createPlatformCamera(this, gstElement);
}

QGStreamerVideoSource::~QGStreamerVideoSource() = default;

bool QGStreamerVideoSource::isActive() const
{
    Q_D(const QGStreamerVideoSource);
    return d->platformCamera && d->platformCamera->isActive();
}

QString QGStreamerVideoSource::gstBinDescription() const
{
    Q_D(const QGStreamerVideoSource);
    return d->gstBinDescription;
}

GstElement *QGStreamerVideoSource::gstElement() const
{
    Q_D(const QGStreamerVideoSource);
    return d->platformCamera ? d->platformCamera->rawGstElement() : nullptr;
}

void QGStreamerVideoSource::setActive(bool active)
{
    Q_D(const QGStreamerVideoSource);
    if (d->platformCamera)
        d->platformCamera->setActive(active);
}

QPlatformCamera *QGStreamerVideoSource::platformVideoSource() const
{
    Q_D(const QGStreamerVideoSource);
    return d->platformCamera;
}

QMediaCaptureSession *QGStreamerVideoSource::captureSession() const
{
    Q_D(const QGStreamerVideoSource);
    return d->captureSession;
}

void QGStreamerVideoSource::setCaptureSession(QMediaCaptureSession *session)
{
    Q_D(QGStreamerVideoSource);
    d->captureSession = session;
}

QT_END_NAMESPACE
