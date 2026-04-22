// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickgstreamervideosource_p.h"

#include <QtMultimedia/private/qgstreamervideosource_p.h>

#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

QQuickGStreamerVideoSource::QQuickGStreamerVideoSource(QObject *parent)
    : QGStreamerVideoSource(parent)
{
}

bool QQuickGStreamerVideoSource::qmlIsActive() const
{
    return m_pendingProperties ? m_pendingProperties->active : isActive();
}

void QQuickGStreamerVideoSource::qmlSetActive(bool active)
{
    if (!m_pendingProperties)
        QGStreamerVideoSource::setActive(active);
    else if (std::exchange(m_pendingProperties->active, active) != active)
        emit activeChanged(active);
}

void QQuickGStreamerVideoSource::setQmlGstBinDescription(QString gstBinDescription)
{
    if (!m_pendingProperties) {
        // TODO: maybe handle exotic case when the required property is
        // set more then once during component's initialization.
        qWarning() << "GstBin's description must be set only once";
        return;
    }

    auto properties = std::move(m_pendingProperties);

    auto *sourcePrivate = QGStreamerVideoSourcePrivate::get(this);
    sourcePrivate->createPlatformCamera(this, std::move(gstBinDescription), properties->active);
}

QT_END_NAMESPACE

#include "moc_qquickgstreamervideosource_p.cpp"
