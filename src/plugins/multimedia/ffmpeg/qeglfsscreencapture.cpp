// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR
// GPL-3.0-only

#include "qeglfsscreencapture_p.h"
#include "qffmpegsurfacecapturegrabber_p.h"
#include "qguiapplication.h"

#include "private/qimagevideobuffer_p.h"

#include <QtOpenGL/private/qopenglcompositor_p.h>

#include <QtQuick/qquickwindow.h>

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
    }

    ~Grabber() override { stop(); }

    QVideoFrameFormat format() { return m_format; }

protected:
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

    QVideoFrameFormat m_format;
};

class QEglfsScreenCapture::QuickGrabber : public Grabber
{
public:
    QuickGrabber(QEglfsScreenCapture &screenCapture, QScreen *screen, QQuickWindow *quickWindow)
        : Grabber(screenCapture, screen), m_quickWindow(quickWindow)
    {
        Q_ASSERT(m_quickWindow);
    }

protected:
    QVideoFrame grabFrame() override
    {
        if (!m_quickWindow) {
            updateError(Error::InternalError, QLatin1String("Window deleted"));
            return {};
        }

        QImage image = m_quickWindow->grabWindow();

        if (image.isNull()) {
            updateError(Error::InternalError, QLatin1String("Image invalid"));
            return {};
        }

        if (!m_format.isValid()) {
            m_format = { image.size(),
                         QVideoFrameFormat::pixelFormatFromImageFormat(image.format()) };
            m_format.setFrameRate(frameRate());
        }

        return QVideoFrame(new QImageVideoBuffer(std::move(image)), m_format);
    }

private:
    QPointer<QQuickWindow> m_quickWindow;
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

    if (!active)
        return true;

    m_grabber = createGrabber();

    if (!m_grabber) {
        // TODO: This could mean that the UI is not started yet, so we should wait and try again,
        // and then give error if still not started. Might not be possible here.
        return false;
    }

    m_grabber->start();
    return true;
}

bool QEglfsScreenCapture::isSupported()
{
    return QGuiApplication::platformName() == QLatin1String("eglfs");
}

std::unique_ptr<QEglfsScreenCapture::Grabber> QEglfsScreenCapture::createGrabber()
{
    auto screen = source<ScreenSource>();
    if (!checkScreenWithError(screen))
        return nullptr;

    QOpenGLCompositor *compositor = QOpenGLCompositor::instance();

    if (compositor->context()) {
        // Create OpenGL grabber
        if (!compositor->targetWindow()) {
            updateError(Error::CaptureFailed,
                        QLatin1String("Target window is not set for OpenGL compositor"));
            return nullptr;
        }

        return std::make_unique<Grabber>(*this, screen);
    }

    // Check for QQuickWindow
    auto windows = QGuiApplication::topLevelWindows();
    auto it = std::find_if(windows.begin(), windows.end(), [screen](QWindow *window) {
        auto quickWindow = qobject_cast<QQuickWindow *>(window);
        if (!quickWindow)
            return false;

        return quickWindow->screen() == screen;
    });

    if (it != windows.end()) {
        // Create grabber that calls QQuickWindow::grabWindow
        return std::make_unique<QuickGrabber>(*this, screen, qobject_cast<QQuickWindow *>(*it));
    }

    updateError(Error::CaptureFailed, QLatin1String("No existing OpenGL context or QQuickWindow"));
    return nullptr;
}

QT_END_NAMESPACE
