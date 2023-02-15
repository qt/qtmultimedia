// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "platform/qplatformscreencapture_p.h"
#include "qvideoframe.h"
#include "qdebug.h"

QT_BEGIN_NAMESPACE

QPlatformScreenCapture::QPlatformScreenCapture(QScreenCapture *screenCapture)
    : QObject(screenCapture), m_screenCapture(screenCapture)
{
    qRegisterMetaType<QVideoFrame>();
}

void QPlatformScreenCapture::setWindow(QWindow *w)
{
    if (w) {
        emit m_screenCapture->errorOccurred(QScreenCapture::WindowCapturingNotSupported,
                                            QLatin1String("Window capture is not supported"));
    }
}

QWindow *QPlatformScreenCapture::window() const
{
    return nullptr;
}

void QPlatformScreenCapture::setWindowId(WId id)
{
    if (id) {
        emit m_screenCapture->errorOccurred(QScreenCapture::WindowCapturingNotSupported,
                                            QLatin1String("Window capture is not supported"));
    }
}

WId QPlatformScreenCapture::windowId() const
{
    return 0;
}

QScreenCapture::Error QPlatformScreenCapture::error() const
{
    return m_error;
}
QString QPlatformScreenCapture::errorString() const
{
    return m_errorString;
}

QScreenCapture *QPlatformScreenCapture::screenCapture() const
{
    return m_screenCapture;
}

std::optional<int> QPlatformScreenCapture::ffmpegHWPixelFormat() const
{
    return {};
}

void QPlatformScreenCapture::updateError(QScreenCapture::Error error, const QString &errorString)
{
    bool changed = error != m_error || errorString != m_errorString;
    m_error = error;
    m_errorString = errorString;
    if (changed) {
        if (m_error != QScreenCapture::NoError)
            emit m_screenCapture->errorOccurred(error, errorString);
        else
            qWarning() << "Screen capture fail:" << error << "," << errorString;

        emit m_screenCapture->errorChanged();
    }
}

QT_END_NAMESPACE
