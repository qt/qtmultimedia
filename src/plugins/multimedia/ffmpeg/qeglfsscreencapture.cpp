// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR
// GPL-3.0-only

#include "qeglfsscreencapture_p.h"
#include "qffmpegsurfacecapturegrabber_p.h"
#include "qguiapplication.h"

#include "private/qimagevideobuffer_p.h"

#include <QtOpenGL/private/qopenglcompositor_p.h>

QT_BEGIN_NAMESPACE

class QEglfsScreenCapture::Grabber : public QFFmpegSurfaceCaptureGrabber
{
public:
    Grabber(QEglfsScreenCapture &screenCapture, QScreen *screen)
        : QFFmpegSurfaceCaptureGrabber(false)
    {
        addFrameCallback(screenCapture, &QEglfsScreenCapture::newVideoFrame);
        connect(this, &Grabber::errorUpdated, &screenCapture, &QEglfsScreenCapture::updateError);
        // Limit frame rate to 30 fps for performance reasons, to be reviewed at the next optimization round
        setFrameRate(std::min(screen->refreshRate(), 30.0));
        start();
    }

    ~Grabber() override { stop(); }

    QVideoFrameFormat format() { return m_format; }

    QVideoFrame grabFrame() override
    {
        QOpenGLCompositor *compositor = QOpenGLCompositor::instance();
        QImage img = compositor->grab();

        if (img.isNull()) {
            updateError(Error::InternalError, QLatin1String("Null image captured"));
            return {};
        }

        if (!m_format.isValid()) {
            // This is a hack to use RGBX8888 video frame format for RGBA8888_Premultiplied image format,
            // due to the lack of QVideoFrameFormat::Format_RGBA8888_Premultiplied
            auto videoFrameFormat = img.format() == QImage::Format_RGBA8888_Premultiplied
                    ? QVideoFrameFormat::Format_RGBX8888
                    : QVideoFrameFormat::pixelFormatFromImageFormat(img.format());
            m_format = { img.size(), videoFrameFormat };
            m_format.setFrameRate(frameRate());
        }

        return QVideoFrame(new QImageVideoBuffer(std::move(img)), m_format);
    }

private:
    QVideoFrameFormat m_format;
};

QEglfsScreenCapture::QEglfsScreenCapture() : QPlatformSurfaceCapture(ScreenSource{}) { }

QEglfsScreenCapture::~QEglfsScreenCapture() = default;

QVideoFrameFormat QEglfsScreenCapture::frameFormat() const
{
    return m_grabber ? m_grabber->format() : QVideoFrameFormat();
}

bool QEglfsScreenCapture::setActiveInternal(bool active)
{
    if (static_cast<bool>(m_grabber) == active)
        return true;

    if (m_grabber)
        m_grabber.reset();
    else {
        auto screen = source<ScreenSource>();
        if (!checkScreenWithError(screen))
            return false;

        QOpenGLCompositor *compositor = QOpenGLCompositor::instance();
        if (!compositor->context()) {
            updateError(Error::CaptureFailed, QLatin1String("OpenGL context is not found"));
            return false;
        }

        if (!compositor->targetWindow()) {
            updateError(Error::CaptureFailed, QLatin1String("Target window is not set for OpenGL compositor"));
            return false;
        }

        // TODO Add check to differentiate between uninitialized UI and QML
        // If UI not started, wait and try again, and then give error if still not started.
        // If QML, give not supported error for now.

        m_grabber = std::make_unique<Grabber>(*this, screen);
    }

    return static_cast<bool>(m_grabber) == active;
}

bool QEglfsScreenCapture::isSupported()
{
    return QGuiApplication::platformName() == QLatin1String("eglfs");
}

QT_END_NAMESPACE
