// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qx11surfacecapture_p.h"
#include "qffmpegsurfacecapturegrabber_p.h"

#include <qvideoframe.h>
#include <qscreen.h>
#include <qwindow.h>
#include <qdebug.h>
#include <qguiapplication.h>
#include <qloggingcategory.h>

#include "private/qcapturablewindow_p.h"
#include "private/qmemoryvideobuffer_p.h"
#include "private/qvideoframeconversionhelper_p.h"
#include "private/qvideoframe_p.h"

#include <X11/Xlib.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xrandr.h>

#include <optional>

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(qLcX11SurfaceCapture, "qt.multimedia.ffmpeg.qx11surfacecapture");

namespace {

void destroyXImage(XImage* image) {
    XDestroyImage(image); // macro
}

template <typename T, typename D>
std::unique_ptr<T, D> makeXUptr(T* ptr, D deleter) {
   return std::unique_ptr<T, D>(ptr, deleter);
}

int screenNumberByName(Display *display, const QString &name)
{
    int size = 0;
    auto monitors = makeXUptr(
            XRRGetMonitors(display, XDefaultRootWindow(display), true, &size),
                &XRRFreeMonitors);
    const auto end = monitors.get() + size;
    auto found = std::find_if(monitors.get(), end, [&](const XRRMonitorInfo &info) {
        auto atomName = makeXUptr(XGetAtomName(display, info.name), &XFree);
        return atomName && name == QString::fromUtf8(atomName.get());
    });

    return found == end ? -1 : std::distance(monitors.get(), found);
}

QVideoFrameFormat::PixelFormat xImagePixelFormat(const XImage &image)
{
    if (image.bits_per_pixel != 32) return QVideoFrameFormat::Format_Invalid;

    if (image.red_mask == 0xff0000 &&
        image.green_mask == 0xff00 &&
        image.blue_mask == 0xff)
        return QVideoFrameFormat::Format_BGRX8888;

    if (image.red_mask == 0xff00 &&
        image.green_mask == 0xff0000 &&
        image.blue_mask == 0xff000000)
        return QVideoFrameFormat::Format_XBGR8888;

    if (image.blue_mask == 0xff0000 &&
        image.green_mask == 0xff00 &&
        image.red_mask == 0xff)
        return QVideoFrameFormat::Format_RGBX8888;

    if (image.red_mask == 0xff00 &&
        image.green_mask == 0xff0000 &&
        image.blue_mask == 0xff000000)
        return QVideoFrameFormat::Format_XRGB8888;

    return QVideoFrameFormat::Format_Invalid;
}

} // namespace

class QX11SurfaceCapture::Grabber : private QFFmpegSurfaceCaptureGrabber
{
public:
    static std::unique_ptr<Grabber> create(QX11SurfaceCapture &capture, QScreen *screen)
    {
        std::unique_ptr<Grabber> result(new Grabber(capture));
        return result->init(screen) ? std::move(result) : nullptr;
    }

    static std::unique_ptr<Grabber> create(QX11SurfaceCapture &capture, WId wid)
    {
        std::unique_ptr<Grabber> result(new Grabber(capture));
        return result->init(wid) ? std::move(result) : nullptr;
    }

    ~Grabber() override
    {
        stop();

        detachShm();
    }

    const QVideoFrameFormat &format() const { return m_format; }

private:
    Grabber(QX11SurfaceCapture &capture)
    {
        addFrameCallback(capture, &QX11SurfaceCapture::newVideoFrame);
        connect(this, &Grabber::errorUpdated, &capture, &QX11SurfaceCapture::updateError);
    }

    bool createDisplay()
    {
        if (!m_display)
            m_display.reset(XOpenDisplay(nullptr));

        if (!m_display)
            updateError(QPlatformSurfaceCapture::InternalError,
                        QLatin1String("Cannot open X11 display"));

        return m_display != nullptr;
    }

    bool init(WId wid)
    {
        if (auto screen = QGuiApplication::primaryScreen())
            setFrameRate(screen->refreshRate());

        return createDisplay() && initWithXID(static_cast<XID>(wid));
    }

    bool init(QScreen *screen)
    {
        if (!screen) {
            updateError(QPlatformSurfaceCapture::NotFound, QLatin1String("Screen Not Found"));
            return false;
        }

        if (!createDisplay())
            return false;

        auto screenNumber = screenNumberByName(m_display.get(), screen->name());

        if (screenNumber < 0)
            return false;

        setFrameRate(screen->refreshRate());

        return initWithXID(RootWindow(m_display.get(), screenNumber));
    }

    bool initWithXID(XID xid)
    {
        m_xid = xid;

        if (update()) {
            start();
            return true;
        }

        return false;
    }

    void detachShm()
    {
        if (std::exchange(m_attached, false)) {
            XShmDetach(m_display.get(), &m_shmInfo);
            shmdt(m_shmInfo.shmaddr);
            shmctl(m_shmInfo.shmid, IPC_RMID, nullptr);
        }
    }

    void attachShm()
    {
        Q_ASSERT(!m_attached);

        m_shmInfo.shmid =
                shmget(IPC_PRIVATE, m_xImage->bytes_per_line * m_xImage->height, IPC_CREAT | 0777);

        if (m_shmInfo.shmid == -1)
            return;

        m_shmInfo.readOnly = false;
        m_shmInfo.shmaddr = m_xImage->data = (char *)shmat(m_shmInfo.shmid, nullptr, 0);

        m_attached = XShmAttach(m_display.get(), &m_shmInfo);
    }

    bool update()
    {
        XWindowAttributes wndattr = {};
        if (XGetWindowAttributes(m_display.get(), m_xid, &wndattr) == 0) {
            updateError(QPlatformSurfaceCapture::CaptureFailed,
                        QLatin1String("Cannot get window attributes"));
            return false;
        }

        // TODO: if capture windows, we should adjust offsets and size if
        // the window is out of the screen borders
        // m_xOffset = ...
        // m_yOffset = ...

        // check window params for the root window as well since
        // it potentially can be changed (e.g. on VM with resizing)
        if (!m_xImage || wndattr.width != m_xImage->width || wndattr.height != m_xImage->height
            || wndattr.depth != m_xImage->depth || wndattr.visual->visualid != m_visualID) {

            qCDebug(qLcX11SurfaceCapture) << "recreate ximage: " << wndattr.width << wndattr.height
                                          << wndattr.depth << wndattr.visual->visualid;

            detachShm();
            m_xImage.reset();

            m_visualID = wndattr.visual->visualid;
            m_xImage.reset(XShmCreateImage(m_display.get(), wndattr.visual, wndattr.depth, ZPixmap,
                                           nullptr, &m_shmInfo, wndattr.width, wndattr.height));

            if (!m_xImage) {
                updateError(QPlatformSurfaceCapture::CaptureFailed,
                            QLatin1String("Cannot create image"));
                return false;
            }

            const auto pixelFormat = xImagePixelFormat(*m_xImage);

            // TODO: probably, add a converter instead
            if (pixelFormat == QVideoFrameFormat::Format_Invalid) {
                updateError(QPlatformSurfaceCapture::CaptureFailed,
                            QLatin1String("Not handled pixel format, bpp=")
                                    + QString::number(m_xImage->bits_per_pixel));
                return false;
            }

            attachShm();

            if (!m_attached) {
                updateError(QPlatformSurfaceCapture::CaptureFailed,
                            QLatin1String("Cannot attach shared memory"));
                return false;
            }

            QVideoFrameFormat format(QSize(m_xImage->width, m_xImage->height), pixelFormat);
            format.setStreamFrameRate(frameRate());
            m_format = format;
        }

        return m_attached;
    }

protected:
    QVideoFrame grabFrame() override
    {
        if (!update())
            return {};

        if (!XShmGetImage(m_display.get(), m_xid, m_xImage.get(), m_xOffset, m_yOffset,
                          AllPlanes)) {
            updateError(QPlatformSurfaceCapture::CaptureFailed,
                        QLatin1String(
                                "Cannot get ximage; the window may be out of the screen borders"));
            return {};
        }

        QByteArray data(m_xImage->bytes_per_line * m_xImage->height, Qt::Uninitialized);

        const auto pixelSrc = reinterpret_cast<const uint32_t *>(m_xImage->data);
        const auto pixelDst = reinterpret_cast<uint32_t *>(data.data());
        const auto pixelCount = data.size() / 4;
        const auto xImageAlphaVaries = false; // In known cases it doesn't vary - it's 0xff or 0xff

        qCopyPixelsWithAlphaMask(pixelDst, pixelSrc, pixelCount, m_format.pixelFormat(),
                                       xImageAlphaVaries);

        auto buffer = std::make_unique<QMemoryVideoBuffer>(data, m_xImage->bytes_per_line);
        return QVideoFramePrivate::createFrame(std::move(buffer), m_format);
    }

private:
    std::optional<QPlatformSurfaceCapture::Error> m_prevGrabberError;
    XID m_xid = None;
    int m_xOffset = 0;
    int m_yOffset = 0;
    std::unique_ptr<Display, decltype(&XCloseDisplay)> m_display{ nullptr, &XCloseDisplay };
    std::unique_ptr<XImage, decltype(&destroyXImage)> m_xImage{ nullptr, &destroyXImage };
    XShmSegmentInfo m_shmInfo;
    bool m_attached = false;
    VisualID m_visualID = None;
    QVideoFrameFormat m_format;
};

QX11SurfaceCapture::QX11SurfaceCapture(Source initialSource)
    : QPlatformSurfaceCapture(initialSource)
{
    // For debug
    //  XSetErrorHandler([](Display *, XErrorEvent * e) {
    //      qDebug() << "error handler" << e->error_code;
    //      return 0;
    //  });
}

QX11SurfaceCapture::~QX11SurfaceCapture() = default;

QVideoFrameFormat QX11SurfaceCapture::frameFormat() const
{
    return m_grabber ? m_grabber->format() : QVideoFrameFormat{};
}

bool QX11SurfaceCapture::setActiveInternal(bool active)
{
    qCDebug(qLcX11SurfaceCapture) << "set active" << active;

    if (m_grabber)
        m_grabber.reset();
    else
        std::visit([this](auto source) { activate(source); }, source());

    return static_cast<bool>(m_grabber) == active;
}

void QX11SurfaceCapture::activate(ScreenSource screen)
{
    if (checkScreenWithError(screen))
        m_grabber = Grabber::create(*this, screen);
}

void QX11SurfaceCapture::activate(WindowSource window)
{
    auto handle = QCapturableWindowPrivate::handle(window);
    m_grabber = Grabber::create(*this, handle ? handle->id : 0);
}

bool QX11SurfaceCapture::isSupported()
{
    return qgetenv("XDG_SESSION_TYPE").compare(QLatin1String("x11"), Qt::CaseInsensitive) == 0;
}

QT_END_NAMESPACE
