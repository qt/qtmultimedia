// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qgdiwindowcapture_p.h"

#include "qvideoframe.h"
#include "qffmpegsurfacecapturegrabber_p.h"
#include "private/qcapturablewindow_p.h"
#include "private/qmemoryvideobuffer_p.h"
#include "private/qvideoframe_p.h"

#include <qt_windows.h>
#include <QtCore/qloggingcategory.h>

Q_STATIC_LOGGING_CATEGORY(qLcGdiWindowCapture, "qt.multimedia.ffmpeg.gdiwindowcapture");

QT_BEGIN_NAMESPACE

class QGdiWindowCapture::Grabber : public QFFmpegSurfaceCaptureGrabber
{
public:
    static std::unique_ptr<Grabber> create(QGdiWindowCapture &capture, HWND hWnd)
    {
        auto hdcWindow = GetDC(hWnd);
        if (!hdcWindow) {
            capture.updateError(QPlatformSurfaceCapture::CaptureFailed,
                                QLatin1String("Cannot create a window drawing context"));
            return nullptr;
        }

        auto hdcMem = CreateCompatibleDC(hdcWindow);

        if (!hdcMem) {
            capture.updateError(QPlatformSurfaceCapture::CaptureFailed,
                                QLatin1String("Cannot create a compatible drawing context"));
            return nullptr;
        }

        std::unique_ptr<Grabber> result(new Grabber(capture, hWnd, hdcWindow, hdcMem));
        if (!result->update())
            return nullptr;

        result->start();
        return result;
    }

    ~Grabber() override
    {
        stop();

        if (m_hBitmap)
            DeleteObject(m_hBitmap);

        if (m_hdcMem)
            DeleteDC(m_hdcMem);

        if (m_hdcWindow)
            ReleaseDC(m_hwnd, m_hdcWindow);
    }

    QVideoFrameFormat format() const { return m_format; }

private:
    Grabber(QGdiWindowCapture &capture, HWND hWnd, HDC hdcWindow, HDC hdcMem)
        : m_hwnd(hWnd), m_hdcWindow(hdcWindow), m_hdcMem(hdcMem)
    {
        if (auto rate = GetDeviceCaps(hdcWindow, VREFRESH); rate > 0)
            setFrameRate(rate);

        addFrameCallback(capture, &QGdiWindowCapture::newVideoFrame);
        connect(this, &Grabber::errorUpdated, &capture, &QGdiWindowCapture::updateError);
    }

    bool update()
    {
        RECT windowRect{};
        if (!GetWindowRect(m_hwnd, &windowRect)) {
            updateError(QPlatformSurfaceCapture::CaptureFailed,
                        QLatin1String("Cannot get window size"));
            return false;
        }

        const QSize size{ windowRect.right - windowRect.left, windowRect.bottom - windowRect.top };

        if (m_format.isValid() && size == m_format.frameSize() && m_hBitmap)
            return true;

        if (m_hBitmap)
            DeleteObject(std::exchange(m_hBitmap, nullptr));

        if (size.isEmpty()) {
            m_format = {};
            updateError(QPlatformSurfaceCapture::CaptureFailed,
                        QLatin1String("Invalid window size"));
            return false;
        }

        m_hBitmap = CreateCompatibleBitmap(m_hdcWindow, size.width(), size.height());

        if (!m_hBitmap) {
            m_format = {};
            updateError(QPlatformSurfaceCapture::CaptureFailed,
                        QLatin1String("Cannot create a compatible bitmap"));
            return false;
        }

        QVideoFrameFormat format(size, QVideoFrameFormat::Format_BGRX8888);
        format.setStreamFrameRate(frameRate());
        m_format = format;
        return true;
    }

    QVideoFrame grabFrame() override
    {
        if (!update())
            return {};

        const auto oldBitmap = SelectObject(m_hdcMem, m_hBitmap);
        auto deselect = qScopeGuard([&]() { SelectObject(m_hdcMem, oldBitmap); });

        const auto size = m_format.frameSize();

        if (!BitBlt(m_hdcMem, 0, 0, size.width(), size.height(), m_hdcWindow, 0, 0, SRCCOPY)) {
            updateError(QPlatformSurfaceCapture::CaptureFailed,
                        QLatin1String("Cannot copy image to the compatible DC"));
            return {};
        }

        BITMAPINFO info{};
        auto &header = info.bmiHeader;
        header.biSize = sizeof(BITMAPINFOHEADER);
        header.biWidth = size.width();
        header.biHeight = -size.height(); // negative height to ensure top-down orientation
        header.biPlanes = 1;
        header.biBitCount = 32;
        header.biCompression = BI_RGB;

        const auto bytesPerLine = size.width() * 4;

        QByteArray array(size.height() * bytesPerLine, Qt::Uninitialized);

        const auto copiedHeight = GetDIBits(m_hdcMem, m_hBitmap, 0, size.height(), array.data(), &info, DIB_RGB_COLORS);
        if (copiedHeight != size.height()) {
            qCWarning(qLcGdiWindowCapture) << copiedHeight << "lines have been copied, expected:" << size.height();
           // In practice, it might fail randomly first time after start. So we don't consider it as an error.
           // TODO: investigate reasons and properly handle the error
           // updateError(QPlatformSurfaceCapture::CaptureFailed,
           //             QLatin1String("Cannot get raw image data"));
            return {};
        }

        if (header.biWidth != size.width() || header.biHeight != -size.height()
            || header.biPlanes != 1 || header.biBitCount != 32 || header.biCompression != BI_RGB) {
            updateError(QPlatformSurfaceCapture::CaptureFailed,
                        QLatin1String("Output bitmap info is unexpected"));
            return {};
        }

        return QVideoFramePrivate::createFrame(
                std::make_unique<QMemoryVideoBuffer>(std::move(array), bytesPerLine), m_format);
    }

private:
    HWND m_hwnd = {};
    QVideoFrameFormat m_format;
    HDC m_hdcWindow = {};
    HDC m_hdcMem = {};
    HBITMAP m_hBitmap = {};
};

QGdiWindowCapture::QGdiWindowCapture() : QPlatformSurfaceCapture(WindowSource{}) { }

QGdiWindowCapture::~QGdiWindowCapture() = default;

QVideoFrameFormat QGdiWindowCapture::frameFormat() const
{
    return m_grabber ? m_grabber->format() : QVideoFrameFormat();
}

bool QGdiWindowCapture::setActiveInternal(bool active)
{
    if (active == static_cast<bool>(m_grabber))
        return true;

    if (m_grabber) {
        m_grabber.reset();
    } else {
        auto window = source<WindowSource>();
        auto handle = QCapturableWindowPrivate::handle(window);

        m_grabber = Grabber::create(*this, reinterpret_cast<HWND>(handle ? handle->id : 0));
    }

    return static_cast<bool>(m_grabber) == active;
}

QT_END_NAMESPACE
