// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qx11screencapture_p.h"

#include <qvideoframe.h>
#include <qthread.h>
#include <qscreen.h>
#include <qwindow.h>
#include <qtimer.h>
#include <qdebug.h>
#include <QGuiApplication>
#include <qloggingcategory.h>

#include "private/qabstractvideobuffer_p.h"

#include <X11/Xlib.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xrandr.h>

#include <optional>

QT_BEGIN_NAMESPACE

static Q_LOGGING_CATEGORY(qLcX11ScreenCapture, "qt.multimedia.ffmpeg.x11screencapture")

namespace {

const qreal MaxFrameRate = 60.;

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

class DataVideoBuffer : public QAbstractVideoBuffer
{
public:
    DataVideoBuffer(const char *data, int bytesPerLine, int size)
        : QAbstractVideoBuffer(QVideoFrame::NoHandle),
          m_data(data, size),
          m_size(size),
          m_bytesPerLine(bytesPerLine)
    {
    }

    QVideoFrame::MapMode mapMode() const override { return m_mapMode; }

    MapData map(QVideoFrame::MapMode mode) override
    {
        MapData mapData;
        if (m_mapMode == QVideoFrame::NotMapped) {
            m_mapMode = mode;

            mapData.nPlanes = 1;
            mapData.bytesPerLine[0] = m_bytesPerLine;
            mapData.data[0] = (uchar *)m_data.data();
            mapData.size[0] = m_size;
        }

        return mapData;
    }

    void unmap() override { m_mapMode = QVideoFrame::NotMapped; }

private:
    QByteArray m_data;
    QVideoFrame::MapMode m_mapMode = QVideoFrame::NotMapped;
    int m_size;
    int m_bytesPerLine;
};

} // namespace

class QX11ScreenCapture::Grabber : private QThread
{
public:
    static std::unique_ptr<Grabber> create(QX11ScreenCapture &capture, QScreen *screen)
    {
        std::unique_ptr<Grabber> result(new Grabber(capture));
        return result->init(screen) ? std::move(result) : nullptr;
    }

    static std::unique_ptr<Grabber> create(QX11ScreenCapture &capture, WId wid)
    {
        std::unique_ptr<Grabber> result(new Grabber(capture));
        return result->init(wid) ? std::move(result) : nullptr;
    }

    ~Grabber() override
    {
        quit();
        wait();

        detachShm();
    }

    const QVideoFrameFormat &format() const { return m_format; }

private:
    Grabber(QX11ScreenCapture &capture) : m_capture(capture) { }

    bool createDisplay()
    {
        if (!m_display)
            m_display.reset(XOpenDisplay(nullptr));

        return m_display != nullptr;
    }

    bool init(WId wid) {
        if (auto screen = QGuiApplication::primaryScreen())
            m_frameRate = std::min(screen->refreshRate(), MaxFrameRate);

        return createDisplay() && initWithXID(static_cast<XID>(wid));
    }

    bool init(QScreen *screen)
    {
        if (!createDisplay())
            return false;

        auto screenNumber = screenNumberByName(m_display.get(), screen->name());

        if (screenNumber < 0)
            return false;

        m_frameRate = std::min(screen->refreshRate(), MaxFrameRate);

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
            shmctl(m_shmInfo.shmid, IPC_RMID, 0);
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
        m_shmInfo.shmaddr = m_xImage->data = (char *)shmat(m_shmInfo.shmid, 0, 0);

        m_attached = XShmAttach(m_display.get(), &m_shmInfo);
    }

    bool update()
    {
        XWindowAttributes wndattr = {};
        if (XGetWindowAttributes(m_display.get(), m_xid, &wndattr) == 0) {
            updateError(QScreenCapture::CaptureFailed, QLatin1String("Cannot get window attributes"));
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

            qCDebug(qLcX11ScreenCapture) << "recreate ximage: " << wndattr.width << wndattr.height << wndattr.depth
                     << wndattr.visual->visualid;

            detachShm();
            m_xImage.reset();

            m_visualID = wndattr.visual->visualid;
            m_xImage.reset(XShmCreateImage(m_display.get(), wndattr.visual, wndattr.depth, ZPixmap,
                                           nullptr, &m_shmInfo, wndattr.width, wndattr.height));

            if (!m_xImage) {
                updateError(QScreenCapture::CaptureFailed, QLatin1String("Cannot create image"));
                return false;
            }

            const auto pixelFormat = xImagePixelFormat(*m_xImage);

            // TODO: probably, add a converter instead
            if (pixelFormat == QVideoFrameFormat::Format_Invalid) {
                updateError(QScreenCapture::CaptureFailed,
                            QLatin1String("Not handled pixel format, bpp=")
                                    + QString::number(m_xImage->bits_per_pixel));
                return false;
            }

            attachShm();

            if (!m_attached) {
                updateError(QScreenCapture::CaptureFailed,
                            QLatin1String("Cannot attach shared memory"));
                return false;
            }

            m_format = QVideoFrameFormat(QSize(m_xImage->width, m_xImage->height),
                                         pixelFormat);
            m_format.setFrameRate(m_frameRate);
        }

        return m_attached;
    }

private:
    void updateError(QScreenCapture::Error error, const QString &description)
    {
        if (error != QScreenCapture::NoError) {
            if (m_timer)
                m_timer->setInterval(1000);
        }

        const auto prevError = std::exchange(m_prevGrabberError, error);

        if (error != QScreenCapture::NoError || prevError != QScreenCapture::NoError) {
            QMetaObject::invokeMethod(&m_capture,
                                      std::bind(&QPlatformScreenCapture::updateError, &m_capture,
                                                error, description));
        }
    }

    void run() override
    {
        m_timer = std::make_unique<QTimer>();
        auto deleter = qScopeGuard([&]() { m_timer.reset(); });
        m_timer->setTimerType(Qt::PreciseTimer);
        QElapsedTimer elapsedTimer;
        qint64 lastFrameTime = 0;

        m_timer->callOnTimeout([&]() { grab(elapsedTimer, lastFrameTime); });
        m_timer->start();

        exec();
    }

    void grab(const QElapsedTimer &elapsedTimer, qint64 &lastFrameTime)
    {
        m_timer->setInterval(1000 / m_frameRate);

        if (!update())
            return;

        if (!XShmGetImage(m_display.get(), m_xid, m_xImage.get(), m_xOffset, m_yOffset,
                          AllPlanes)) {
            updateError(QScreenCapture::CaptureFailed,
                        QLatin1String(
                                "Cannot get ximage; the window may be out of the screen borders"));
            return;
        }

        auto buffer = new DataVideoBuffer(m_xImage->data, m_xImage->bytes_per_line,
                                          m_xImage->bytes_per_line * m_xImage->height);

        QVideoFrame frame(buffer, m_format);

        const auto endTime = elapsedTimer.nsecsElapsed() / 1000;
        frame.setStartTime(lastFrameTime);
        frame.setEndTime(endTime);
        emit m_capture.newVideoFrame(frame);

        updateError(QScreenCapture::NoError, {});

        lastFrameTime = endTime;
    }

private:
    QX11ScreenCapture &m_capture;
    std::optional<QScreenCapture::Error> m_prevGrabberError;
    XID m_xid = None;
    int m_xOffset = 0;
    int m_yOffset = 0;
    std::unique_ptr<Display, decltype(&XCloseDisplay)> m_display{nullptr, &XCloseDisplay};
    std::unique_ptr<XImage, decltype(&destroyXImage)> m_xImage{nullptr, &destroyXImage};
    XShmSegmentInfo m_shmInfo;
    bool m_attached = false;
    qreal m_frameRate = MaxFrameRate;
    VisualID m_visualID = None;
    std::unique_ptr<QTimer> m_timer;
    QVideoFrameFormat m_format;
};

QX11ScreenCapture::QX11ScreenCapture(QScreenCapture *screenCapture)
    : QFFmpegScreenCaptureBase(screenCapture)
{
    // For debug
    //  XSetErrorHandler([](Display *, XErrorEvent * e) {
    //      qDebug() << "error handler" << e->error_code;
    //      return 0;
    //  });
}

QX11ScreenCapture::~QX11ScreenCapture() = default;

QVideoFrameFormat QX11ScreenCapture::format() const
{
    return m_grabber ? m_grabber->format() : QVideoFrameFormat{};
}

bool QX11ScreenCapture::setActiveInternal(bool active)
{
    qCDebug(qLcX11ScreenCapture) << "set active" << active;

    if (active) {
        if (auto wid = window() ? window()->winId() : windowId())
            m_grabber = Grabber::create(*this, wid);
        else if (auto scrn = screen() ? screen() : QGuiApplication::primaryScreen())
            m_grabber = Grabber::create(*this, scrn);
    } else {
        m_grabber.reset();
    }

    return static_cast<bool>(m_grabber) == active;
}

bool QX11ScreenCapture::isSupported()
{
    return qgetenv("XDG_SESSION_TYPE").compare(QLatin1String("x11"), Qt::CaseInsensitive) == 0;
}

QT_END_NAMESPACE
