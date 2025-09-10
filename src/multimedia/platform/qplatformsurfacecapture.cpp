// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "platform/qplatformsurfacecapture_p.h"
#include "qvideoframe.h"
#include "qguiapplication.h"
#include "qdebug.h"

QT_BEGIN_NAMESPACE

QPlatformSurfaceCapture::QPlatformSurfaceCapture(Source initialSource) : m_source(initialSource)
{
    Q_ASSERT(std::visit([](auto source) { return source == decltype(source){}; }, initialSource));
    qRegisterMetaType<QVideoFrame>();
}

void QPlatformSurfaceCapture::setActive(bool active)
{
    if (m_active == active)
        return;

    if (!setActiveInternal(active))
        return;

    m_active = active;
    emit activeChanged(active);
}

bool QPlatformSurfaceCapture::isActive() const
{
    return m_active;
}

void QPlatformSurfaceCapture::setSource(Source source)
{
    Q_ASSERT(source.index() == m_source.index());

    if (m_source == source)
        return;

    if (m_active)
        setActiveInternal(false);

    m_source = source;

    if (m_active && !setActiveInternal(true)) {
        m_active = false;
        emit activeChanged(false);
    }

    std::visit([this](auto source) { emit sourceChanged(source); }, m_source);
}

QPlatformSurfaceCapture::Error QPlatformSurfaceCapture::error() const
{
    return m_error.code();
}

QString QPlatformSurfaceCapture::errorString() const
{
    return m_error.description();
}

void QPlatformSurfaceCapture::updateError(Error error, const QString &errorString)
{
    m_error.setAndNotify(error, errorString, *this);
}

bool QPlatformSurfaceCapture::checkScreenWithError(ScreenSource &screen)
{
    if (!screen)
        screen = QGuiApplication::primaryScreen();

    if (screen)
        return true;

    updateError(NotFound, QLatin1String("No screens found"));
    return false;
}

QT_END_NAMESPACE

#include "moc_qplatformsurfacecapture_p.cpp"
