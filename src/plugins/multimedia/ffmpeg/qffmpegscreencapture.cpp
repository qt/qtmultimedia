// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qvideoframe.h"
#include "qffmpegscreencapture_p.h"
#include "qscreencapture.h"
#include "qffmpegscreencapturethread_p.h"

#include "private/qabstractvideobuffer_p.h"

#include "qscreen.h"
#include "qmutex.h"
#include "qwaitcondition.h"
#include "qpixmap.h"
#include "qguiapplication.h"
#include "qdatetime.h"
#include "qwindow.h"
#include "qelapsedtimer.h"

#include <QtCore/qloggingcategory.h>

static Q_LOGGING_CATEGORY(qLcFfmpegScreenCapture, "qt.multimedia.ffmpeg.screencapture")

QT_BEGIN_NAMESPACE

namespace {

using WindowUPtr = std::unique_ptr<QWindow>;

class QImageVideoBuffer : public QAbstractVideoBuffer
{
public:
    QImageVideoBuffer(QImage &&image)
        : QAbstractVideoBuffer(QVideoFrame::NoHandle), m_image(std::move(image))
    {
    }

    QVideoFrame::MapMode mapMode() const override
    {
        return m_mapMode;
    }

    MapData map(QVideoFrame::MapMode mode) override
    {
        MapData mapData;
        if (m_mapMode == QVideoFrame::NotMapped && !m_image.isNull() && mode != QVideoFrame::NotMapped) {
            m_mapMode = mode;

            mapData.nPlanes = 1;
            mapData.bytesPerLine[0] = m_image.bytesPerLine();
            mapData.data[0] = m_image.bits();
            mapData.size[0] = m_image.sizeInBytes();
        }

        return mapData;
    }

    void unmap() override
    {
        m_mapMode = QVideoFrame::NotMapped;
    }

    QVideoFrame::MapMode m_mapMode = QVideoFrame::NotMapped;
    QImage m_image;
};

}

class QFFmpegScreenCapture::Grabber : public QFFmpegScreenCaptureThread
{
public:
    Grabber(QFFmpegScreenCapture &capture, QScreen *screen) : Grabber(capture, screen, nullptr)
    {
        Q_ASSERT(screen);
    }

    Grabber(QFFmpegScreenCapture &capture, WindowUPtr window)
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
    Grabber(QFFmpegScreenCapture &capture, QScreen *screen, WindowUPtr window)
        : m_capture(capture), m_screen(screen), m_window(std::move(window))
    {
        connect(qApp, &QGuiApplication::screenRemoved, this, &Grabber::onScreenRemoved);
        addFrameCallback(m_capture, &QFFmpegScreenCapture::newVideoFrame);
        connect(this, &Grabber::errorUpdated, &m_capture, &QFFmpegScreenCapture::updateError);
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
        auto screenGuard = qScopeGuard(std::bind(&Grabber::setScreenRemovingLocked, this, false));

        WId wid = m_window ? m_window->winId() : 0;
        QScreen *screen = m_window ? m_window->screen() : m_screen ? m_screen.data() : nullptr;

        if (!screen) {
            updateError(QScreenCapture::Error::CaptureFailed, "Screen not found");
            return {};
        }

        setFrameRate(screen->refreshRate());

        QPixmap p = screen->grabWindow(wid);
        QImage img = p.toImage();

        QVideoFrameFormat format(img.size(),
                                 QVideoFrameFormat::pixelFormatFromImageFormat(img.format()));
        format.setFrameRate(screen->refreshRate());
        updateFormat(format);

        if (!format.isValid()) {
            updateError(QScreenCapture::Error::CaptureFailed,
                                  "Failed to grab the screen content");
            return {};
        }

        return QVideoFrame(new QImageVideoBuffer(std::move(img)), format);
    }

private:
    QFFmpegScreenCapture &m_capture;
    QPointer<QScreen> m_screen;
    WindowUPtr m_window;

    QMutex m_formatMutex;
    QWaitCondition m_waitForFormat;
    std::optional<QVideoFrameFormat> m_format;

    QMutex m_screenRemovingMutex;
    bool m_screenRemovingLocked = false;
    QWaitCondition m_screenRemovingWc;
};

QFFmpegScreenCapture::QFFmpegScreenCapture(QScreenCapture *screenCapture)
    : QFFmpegScreenCaptureBase(screenCapture)
{
}

QFFmpegScreenCapture::~QFFmpegScreenCapture() = default;

QVideoFrameFormat QFFmpegScreenCapture::frameFormat() const
{
    if (m_grabber)
        return m_grabber->format();
    else
        return {};
}

bool QFFmpegScreenCapture::setActiveInternal(bool active)
{
    if (active == bool(m_grabber))
        return true;

    if (m_grabber) {
        m_grabber.reset();
        return true;
    }

    if (auto wid = window() ? window()->winId() : windowId()) {
        // create a copy of QWindow anyway since Grabber has to own the object
        if (auto wnd = WindowUPtr(QWindow::fromWinId(wid))) {
            if (!wnd->screen()) {
                updateError(QScreenCapture::InternalError,
                            "Window " + QString::number(wid) + " doesn't belong to any screen");
                return false;
            }

            m_grabber = std::make_unique<Grabber>(*this, std::move(wnd));
            m_grabber->start();
            return true;
        } else {
            updateError(QScreenCapture::NotFound,
                        "Window " + QString::number(wid) + "doesn't exist or permissions denied");
            return false;
        }
    }

    if (auto scrn = screen() ? screen() : QGuiApplication::primaryScreen()) {
        m_grabber = std::make_unique<Grabber>(*this, scrn);
        m_grabber->start();
        return true;
    }

    updateError(QScreenCapture::NotFound, "Screen not found");
    return false;
}

void QFFmpegScreenCapture::updateError(QScreenCapture::Error error, const QString &description)
{
    qCDebug(qLcFfmpegScreenCapture) << description;
    QMetaObject::invokeMethod(this, "updateError", Q_ARG(QScreenCapture::Error, error),
                              Q_ARG(QString, description));
}

QT_END_NAMESPACE
