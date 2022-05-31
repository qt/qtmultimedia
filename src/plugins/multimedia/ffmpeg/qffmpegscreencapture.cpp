// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qscreen.h"
#include "qthread.h"
#include "qmutex.h"
#include "qwaitcondition.h"
#include "qpixmap.h"
#include "qguiapplication.h"
#include "qdatetime.h"

#include "qvideoframe.h"
#include "qffmpegvideobuffer_p.h"
#include "qffmpegscreencapture_p.h"
#include "qscreencapture.h"

#include <thread>
#include <chrono>

QT_BEGIN_NAMESPACE

using namespace std::chrono;

class QImageVideoBuffer : public QAbstractVideoBuffer
{
public:
    QImageVideoBuffer(QImage &&image)
        : QAbstractVideoBuffer(QVideoFrame::NoHandle)
        , m_image(image)
    {}

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

class ScreenGrabberActive : public QThread
{
public:
    ScreenGrabberActive(QPlatformScreenCapture &screenCapture, const QString &screenName)
        : m_screenCapture(screenCapture)
        , m_screenName(screenName)
    {}

    void run() override {
        QScreen *screen = nullptr;
        for (auto s : QGuiApplication::screens()) {
            if (s->name() == m_screenName) {
                screen = s;
                break;
            }
        }
        if (!screen) {
            emit m_screenCapture.updateError(QScreenCapture::Error::NotFound,
                                             "Screen not found");
            return;
        }

        const microseconds frameTime(quint64(1000000. / screen->refreshRate()));
        time_point frameStartTime = steady_clock::now();
        time_point frameStopTime = steady_clock::time_point{};

        while (!isInterruptionRequested()) {
            QPixmap p = screen->grabWindow(0);
            QImage img = p.toImage();
            QVideoFrameFormat format(img.size(), QVideoFrameFormat::pixelFormatFromImageFormat(img.format()));

            if (!format.isValid()) {
                emit m_screenCapture.updateError(QScreenCapture::Error::CaptureFailed,
                                                 "Failed to grab the screen content");
                continue;
            }

            if (!m_format.isValid()) {
                m_formatMutex.lock();
                m_format = format;
                m_format.setFrameRate(screen->refreshRate());
                m_formatMutex.unlock();
                m_waitForFormat.wakeAll();
            }

            frameStopTime = steady_clock::now();

            QVideoFrame frame(new QImageVideoBuffer(std::move(img)), format);

            frame.setStartTime(duration_cast<microseconds>(frameStartTime.time_since_epoch()).count());
            frame.setEndTime(duration_cast<microseconds>(frameStopTime.time_since_epoch()).count());
            emit m_screenCapture.newVideoFrame(frame);

            std::this_thread::sleep_until(frameStartTime + frameTime);
            frameStartTime = frameStopTime;
        }
    }

    QVideoFrameFormat format() {
        QMutexLocker locker(&m_formatMutex);
        if (!m_format.isValid())
            m_waitForFormat.wait(&m_formatMutex);
        return m_format;
    }

private:
    QPlatformScreenCapture &m_screenCapture;
    QMutex m_formatMutex;
    QWaitCondition m_waitForFormat;
    QVideoFrameFormat m_format;
    QString m_screenName;
};

QFFmpegScreenCapture::QFFmpegScreenCapture(QScreenCapture *screenCapture)
    : QPlatformScreenCapture(screenCapture)
{
}

QFFmpegScreenCapture::~QFFmpegScreenCapture() = default;

QVideoFrameFormat QFFmpegScreenCapture::format() const
{
    if (m_active)
        return m_active->format();
    else
        return {};
}

void QFFmpegScreenCapture::setScreen(QScreen *screen)
{
    QScreen *oldScreen = m_screen;
    if (oldScreen == screen)
        return;

    bool active = bool(m_active);
    if (active)
        setActiveInternal(false);

    m_screen = screen;
    if (active)
        setActiveInternal(true);

    emit screenCapture()->screenChanged(screen);
}

void QFFmpegScreenCapture::setActiveInternal(bool active)
{
    if (bool(m_active) == active)
        return;

    if (m_active) {
        m_active->requestInterruption();
        m_active->quit();
        m_active->wait();
        m_active.reset();
    } else {
        QScreen *screen = m_screen ? m_screen : QGuiApplication::primaryScreen();
        m_active.reset(new ScreenGrabberActive(*this, screen->name()));
        m_active->start();
    }
}

void QFFmpegScreenCapture::setActive(bool active)
{
    if (bool(m_active) == active)
        return;

    setActiveInternal(active);
    emit screenCapture()->activeChanged(active);
}

QT_END_NAMESPACE
