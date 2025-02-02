// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qvideoframe.h"
#include "qgrabwindowsurfacecapture_p.h"
#include "qscreencapture.h"
#include "qffmpegsurfacecapturegrabber_p.h"

#include "private/qimagevideobuffer_p.h"
#include "private/qcapturablewindow_p.h"
#include "private/qvideoframe_p.h"

#include "qscreen.h"
#include "qmutex.h"
#include "qwaitcondition.h"
#include "qpixmap.h"
#include "qguiapplication.h"
#include "qwindow.h"
#include "qpointer.h"

#include <QtCore/qloggingcategory.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

namespace {

using WindowUPtr = std::unique_ptr<QWindow>;

} // namespace

class QGrabWindowSurfaceCapture::Grabber : public QFFmpegSurfaceCaptureGrabber
{
public:
    Grabber(QGrabWindowSurfaceCapture &capture, QScreen *screen) : Grabber(capture, screen, nullptr)
    {
        Q_ASSERT(screen);
    }

    Grabber(QGrabWindowSurfaceCapture &capture, WindowUPtr window)
        : Grabber(capture, nullptr, std::move(window))
    {
        Q_ASSERT(m_window);
    }

    ~Grabber() override {
        stop();

        Q_ASSERT(!m_screenRemovingLocked);
    }

    const QVideoFrameFormat &format()
    {
        QMutexLocker locker(&m_formatMutex);
        while (!m_format)
            m_waitForFormat.wait(&m_formatMutex);
        return *m_format;
    }

private:
    Grabber(QGrabWindowSurfaceCapture &capture, QScreen *screen, WindowUPtr window)
        : QFFmpegSurfaceCaptureGrabber(
                QGuiApplication::platformName() == QLatin1String("eglfs")
                        ? QFFmpegSurfaceCaptureGrabber::UseCurrentThread
                        : QFFmpegSurfaceCaptureGrabber::CreateGrabbingThread),
          m_capture(capture),
          m_screen(screen),
          m_window(std::move(window))
    {
        connect(qApp, &QGuiApplication::screenRemoved, this, &Grabber::onScreenRemoved);
        addFrameCallback(m_capture, &QGrabWindowSurfaceCapture::newVideoFrame);
        connect(this, &Grabber::errorUpdated, &m_capture, &QGrabWindowSurfaceCapture::updateError);
    }

    void onScreenRemoved(QScreen *screen)
    {
        /* The hack allows to lock screens removing while QScreen::grabWindow is in progress.
         * The current solution works since QGuiApplication::screenRemoved is emitted from
         *     the destructor of QScreen before destruction members of the object.
         * Note, QGuiApplication works with screens in the main thread, and any removing of a screen
         *    must be synchronized with grabbing thread.
         */
        QMutexLocker locker(&m_screenRemovingMutex);

        if (m_screenRemovingLocked) {
            qDebug() << "Screen" << screen->name()
                     << "is removed while screen window grabbing lock is active";
        }

        while (m_screenRemovingLocked)
            m_screenRemovingWc.wait(&m_screenRemovingMutex);
    }

    void setScreenRemovingLocked(bool locked)
    {
        Q_ASSERT(locked != m_screenRemovingLocked);

        {
            QMutexLocker locker(&m_screenRemovingMutex);
            m_screenRemovingLocked = locked;
        }

        if (!locked)
            m_screenRemovingWc.wakeAll();
    }

    void updateFormat(const QVideoFrameFormat &format)
    {
        if (m_format && m_format->isValid())
            return;

        {
            QMutexLocker locker(&m_formatMutex);
            m_format = format;
        }

        m_waitForFormat.wakeAll();
    }

    QVideoFrame grabFrame() override
    {
        setScreenRemovingLocked(true);
        auto screenGuard = qScopeGuard([this] {
            setScreenRemovingLocked(false);
        });

        WId wid = m_window ? m_window->winId() : 0;
        QScreen *screen = m_window ? m_window->screen() : m_screen ? m_screen.data() : nullptr;

        if (!screen) {
            updateError(QPlatformSurfaceCapture::CaptureFailed, u"Screen not found"_s);
            return {};
        }

        setFrameRate(screen->refreshRate());

        QPixmap p = screen->grabWindow(wid);
        auto buffer = std::make_unique<QImageVideoBuffer>(p.toImage());
        const auto img = buffer->underlyingImage();

        QVideoFrameFormat format(img.size(),
                                 QVideoFrameFormat::pixelFormatFromImageFormat(img.format()));
        format.setStreamFrameRate(screen->refreshRate());
        updateFormat(format);

        if (!format.isValid()) {
            updateError(QPlatformSurfaceCapture::CaptureFailed,
                        u"Failed to grab the screen content"_s);
            return {};
        }

        return QVideoFramePrivate::createFrame(std::move(buffer), std::move(format));
    }

private:
    QGrabWindowSurfaceCapture &m_capture;
    QPointer<QScreen> m_screen;
    WindowUPtr m_window;

    QMutex m_formatMutex;
    QWaitCondition m_waitForFormat;
    std::optional<QVideoFrameFormat> m_format;

    QMutex m_screenRemovingMutex;
    bool m_screenRemovingLocked = false;
    QWaitCondition m_screenRemovingWc;
};

QGrabWindowSurfaceCapture::QGrabWindowSurfaceCapture(Source initialSource)
    : QPlatformSurfaceCapture(initialSource)
{
}

QGrabWindowSurfaceCapture::~QGrabWindowSurfaceCapture() = default;

QVideoFrameFormat QGrabWindowSurfaceCapture::frameFormat() const
{
    if (m_grabber)
        return m_grabber->format();
    else
        return {};
}

bool QGrabWindowSurfaceCapture::setActiveInternal(bool active)
{
    if (active == static_cast<bool>(m_grabber))
        return true;

    if (m_grabber)
        m_grabber.reset();
    else
        std::visit([this](auto source) { activate(source); }, source());

    return static_cast<bool>(m_grabber) == active;
}

void QGrabWindowSurfaceCapture::activate(ScreenSource screen)
{
    if (!checkScreenWithError(screen))
        return;

    m_grabber = std::make_unique<Grabber>(*this, screen);
    m_grabber->start();
}

void QGrabWindowSurfaceCapture::activate(WindowSource window)
{
    auto handle = QCapturableWindowPrivate::handle(window);
    auto wid = handle ? handle->id : 0;
    if (auto wnd = WindowUPtr(QWindow::fromWinId(wid))) {
        if (!wnd->screen()) {
            updateError(InternalError,
                        u"Window " + QString::number(wid) + u" doesn't belong to any screen");
        } else {
            m_grabber = std::make_unique<Grabber>(*this, std::move(wnd));
            m_grabber->start();
        }
    } else {
        updateError(NotFound,
                    u"Window " + QString::number(wid) + u"doesn't exist or permissions denied");
    }
}

QT_END_NAMESPACE
