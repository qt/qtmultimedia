// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR
// GPL-3.0-only

#include "qeglfsscreencapture_p.h"
#include "qffmpegsurfacecapturegrabber_p.h"
#include "qguiapplication.h"

QT_BEGIN_NAMESPACE

class QEglfsScreenCapture::Grabber : public QFFmpegSurfaceCaptureGrabber
{
public:
    Grabber(QEglfsScreenCapture &screenCapture, QScreen *screen)
        : QFFmpegSurfaceCaptureGrabber(false)
    {
        setFrameRate(screen->refreshRate());
        addFrameCallback(screenCapture, &QEglfsScreenCapture::newVideoFrame);
        connect(this, &Grabber::errorUpdated, &screenCapture, &QEglfsScreenCapture::updateError);
    }

    ~Grabber() override { stop(); }

    QVideoFrameFormat format() { return m_format; }

    QVideoFrame grabFrame() override
    {
        // To be implemented: take a frame and updare m_format
        return {};
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
    if (m_grabber)
        m_grabber.reset();
    else if (auto screen = source<ScreenSource>(); checkScreenWithError(screen))
        m_grabber = std::make_unique<Grabber>(*this, screen);

    return static_cast<bool>(m_grabber) == active;
}

bool QEglfsScreenCapture::isSupported()
{
    // return QGuiApplication::platformName() == QLatin1String("eglfs"))
    return false;
}

QT_END_NAMESPACE
