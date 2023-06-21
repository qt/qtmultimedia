// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "platform/qplatformsurfacecapture_p.h"
#include "qvideoframe.h"
#include "qdebug.h"

QT_BEGIN_NAMESPACE

QPlatformSurfaceCapture::QPlatformSurfaceCapture(QScreenCapture *screenCapture)
    : QPlatformVideoSource(screenCapture), m_screenCapture(screenCapture)
{
    qRegisterMetaType<QVideoFrame>();
}

void QPlatformSurfaceCapture::setWindow(QWindow *w)
{
    if (w) {
        emit m_screenCapture->errorOccurred(QScreenCapture::InternalError,
                                            QLatin1String("Window capture is not supported"));
    }
}

QWindow *QPlatformSurfaceCapture::window() const
{
    return nullptr;
}

void QPlatformSurfaceCapture::setWindowId(WId id)
{
    if (id) {
        emit m_screenCapture->errorOccurred(QScreenCapture::InternalError,
                                            QLatin1String("Window capture is not supported"));
    }
}

WId QPlatformSurfaceCapture::windowId() const
{
    return 0;
}

QScreenCapture::Error QPlatformSurfaceCapture::error() const
{
    return m_error;
}
QString QPlatformSurfaceCapture::errorString() const
{
    return m_errorString;
}

QScreenCapture *QPlatformSurfaceCapture::screenCapture() const
{
    return m_screenCapture;
}

void QPlatformSurfaceCapture::updateError(QScreenCapture::Error error, const QString &errorString)
{
    bool changed = error != m_error || errorString != m_errorString;
    m_error = error;
    m_errorString = errorString;
    if (changed) {
        if (m_error != QScreenCapture::NoError) {
            emit m_screenCapture->errorOccurred(error, errorString);
            qWarning() << "Screen capture fail:" << error << "," << errorString;
        }

        emit m_screenCapture->errorChanged();
    }
}

QT_END_NAMESPACE

#include "moc_qplatformsurfacecapture_p.cpp"
