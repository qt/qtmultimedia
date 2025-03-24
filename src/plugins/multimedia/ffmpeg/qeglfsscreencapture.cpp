// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qeglfsscreencapture_p.h"

#include "qffmpegsurfacecapturegrabber_p.h"
#include "qguiapplication.h"
#include "qopenglvideobuffer_p.h"
#include "private/qimagevideobuffer_p.h"
#include "private/qvideoframe_p.h"

#include <QtOpenGL/private/qopenglcompositor_p.h>
#include <QtOpenGL/private/qopenglframebufferobject_p.h>

#ifndef QT_NO_QUICK
#include <QtQuick/qquickwindow.h>
#endif

QT_BEGIN_NAMESPACE

class QEglfsScreenCapture::Grabber : public QFFmpegSurfaceCaptureGrabber
{
public:
    Grabber(QEglfsScreenCapture &screenCapture, QScreen *screen)
        : QFFmpegSurfaceCaptureGrabber(QFFmpegSurfaceCaptureGrabber::UseCurrentThread)
    {
        addFrameCallback(screenCapture, &QEglfsScreenCapture::newVideoFrame);
        connect(this, &Grabber::errorUpdated, &screenCapture, &QEglfsScreenCapture::updateError);
        // Limit frame rate to 30 fps for performance reasons,
        // to be reviewed at the next optimization round
        setFrameRate(std::min(screen->refreshRate(), qreal(30.0)));
    }

    ~Grabber() override { stop(); }

    QVideoFrameFormat format() { return m_format; }

protected:
    QVideoFrame grabFrame() override
    {
        auto nativeSize = QOpenGLCompositor::instance()->nativeTargetGeometry().size();
        auto fbo = std::make_unique<QOpenGLFramebufferObject>(nativeSize);

        if (!QOpenGLCompositor::instance()->grabToFrameBufferObject(
                    fbo.get(), QOpenGLCompositor::NotFlipped)) {
            updateError(Error::InternalError, QLatin1String("Couldn't grab to framebuffer object"));
            return {};
        }

        if (!fbo->isValid()) {
            updateError(Error::InternalError, QLatin1String("Framebuffer object invalid"));
            return {};
        }

        auto videoBuffer = std::make_unique<QOpenGLVideoBuffer>(std::move(fbo));

        if (!m_format.isValid()) {
            auto image = videoBuffer->ensureImageBuffer().underlyingImage();
            m_format = { image.size(), QVideoFrameFormat::pixelFormatFromImageFormat(image.format()) };
            m_format.setStreamFrameRate(frameRate());
        }

        return QVideoFramePrivate::createFrame(std::move(videoBuffer), m_format);
    }

    QVideoFrameFormat m_format;
};

#ifndef QT_NO_QUICK
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
            m_format.setStreamFrameRate(frameRate());
        }

        return QVideoFramePrivate::createFrame(
                std::make_unique<QImageVideoBuffer>(std::move(image)), m_format);
    }

private:
    QPointer<QQuickWindow> m_quickWindow;
};
#endif // QT_NO_QUICK

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

#ifndef QT_NO_QUICK
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
#endif // QT_NO_QUICK

    updateError(Error::CaptureFailed, QLatin1String("No existing OpenGL context or QQuickWindow"));
    return nullptr;
}

QT_END_NAMESPACE
