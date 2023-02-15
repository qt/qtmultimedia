// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qavfscreencapture_p.h"
#include <qpointer.h>
#include <qscreencapture.h>
#include <qscreen.h>
#include <qthread.h>
#include <qtimer.h>
#include <QGuiApplication>
#include <private/qrhi_p.h>
#include "qavfsamplebufferdelegate_p.h"

#define AVMediaType XAVMediaType
#include "qffmpeghwaccel_p.h"

extern "C" {
#include <libavutil/hwcontext_videotoolbox.h>
#include <libavutil/hwcontext.h>
}
#undef AVMediaType

#include <ApplicationServices/ApplicationServices.h>
#include <IOKit/graphics/IOGraphicsLib.h>
#include <AppKit/NSScreen.h>
#include <AppKit/NSApplication.h>
#include <AppKit/NSWindow.h>

#include <dispatch/dispatch.h>

namespace {

const auto DefaultCVPixelFormat = kCVPixelFormatType_32BGRA;

// Mac screens often support 120 frames per sec; it looks, this is not
// needed for the capturing now since it just affects CPI without valuable
// advantages. In the future, the frame rate should be customized by
// user's API.
const qreal MaxFrameRate = 60.;

CGDirectDisplayID findDisplayByName(const QString &name)
{
    for (NSScreen *screen in NSScreen.screens) {
        if (name == QString::fromNSString(screen.localizedName))
            return [screen.deviceDescription[@"NSScreenNumber"] unsignedIntValue];
    }
    return kCGNullDirectDisplay;
}

CGWindowID findNativeWindowID(WId wid)
{
    // qtbase functionality sets QWidget::winId to the pointer
    // value of the matching NSView. This is kind of mess,
    // so we're trying resolving it via this lookup.
    for (NSWindow *window in NSApp.windows) {
        if (window.initialFirstResponder == (NSView *)wid)
            return static_cast<CGWindowID>(window.windowNumber);
    }

    return static_cast<CGWindowID>(wid);
}

qreal frameRateForWindow(CGWindowID /*wid*/)
{
    // TODO: detect the frame rate
    // if (window && window.screen) {
    //     CGDirectDisplayID displayID = [window.screen.deviceDescription[@"NSScreenNumber"]
    //     unsignedIntValue]; const auto displayRefreshRate =
    //     CGDisplayModeGetRefreshRate(CGDisplayCopyDisplayMode(displayID)); if (displayRefreshRate > 0 &&
    //     displayRefreshRate < frameRate) frameRate = displayRefreshRate;
    // }

    return MaxFrameRate;
}

}

QT_BEGIN_NAMESPACE

QAVFScreenCapture::QAVFScreenCapture(QScreenCapture *screenCapture)
    : QFFmpegScreenCaptureBase(screenCapture)
{
    CGRequestScreenCaptureAccess();
}

QAVFScreenCapture::~QAVFScreenCapture()
{
    resetCapture();
}

bool QAVFScreenCapture::setActiveInternal(bool active)
{
    if (active) {
        if (!CGPreflightScreenCaptureAccess()) {
            updateError(QScreenCapture::CaptureFailed, QLatin1String("Permissions denied"));
            return false;
        }

        if (auto winId = window() ? window()->winId() : windowId())
            return initWindowCapture(winId);
        else if (auto scrn = screen() ? screen() : QGuiApplication::primaryScreen())
            return initScreenCapture(scrn);
        else {
            updateError(QScreenCapture::NotFound, QLatin1String("Primary screen not found"));
            return false;
        }
    } else {
        resetCapture();
    }

    return true;
}

void QAVFScreenCapture::onNewFrame(const QVideoFrame &frame)
{
    // Since writing of the format is supposed to be only from one thread,
    // the read-only comparison without a mutex is thread-safe
    if (!m_format || m_format != frame.surfaceFormat()) {
        QMutexLocker<QMutex> locker(&m_formatMutex);

        m_format = frame.surfaceFormat();

        locker.unlock();

        m_waitForFormat.notify_one();
    }

    emit newVideoFrame(frame);
}

QVideoFrameFormat QAVFScreenCapture::format() const
{
    if (!m_grabber)
        return {};

    QMutexLocker<QMutex> locker(&m_formatMutex);
    while (!m_format)
        m_waitForFormat.wait(&m_formatMutex);
    return *m_format;
}

std::optional<int> QAVFScreenCapture::ffmpegHWPixelFormat() const
{
    return AV_PIX_FMT_VIDEOTOOLBOX;
}

class QCGImageVideoBuffer : public QAbstractVideoBuffer
{
public:
    QCGImageVideoBuffer(CGImageRef image) : QAbstractVideoBuffer(QVideoFrame::NoHandle)
    {
        auto provider = CGImageGetDataProvider(image);
        m_data = CGDataProviderCopyData(provider);
        m_bytesPerLine = CGImageGetBytesPerRow(image);
    }

    ~QCGImageVideoBuffer() override { CFRelease(m_data); }

    QVideoFrame::MapMode mapMode() const override { return m_mapMode; }

    MapData map(QVideoFrame::MapMode mode) override
    {
        MapData mapData;
        if (m_mapMode == QVideoFrame::NotMapped) {
            m_mapMode = mode;

            mapData.nPlanes = 1;
            mapData.bytesPerLine[0] = static_cast<int>(m_bytesPerLine);
            mapData.data[0] = (uchar *)CFDataGetBytePtr(m_data);
            mapData.size[0] = static_cast<int>(CFDataGetLength(m_data));
        }

        return mapData;
    }

    void unmap() override { m_mapMode = QVideoFrame::NotMapped; }

private:
    QVideoFrame::MapMode m_mapMode = QVideoFrame::NotMapped;
    CFDataRef m_data;
    size_t m_bytesPerLine = 0;
};

class QAVFScreenCapture::Grabber
{
public:
    virtual ~Grabber() = default;
};

class QAVFScreenCapture::WindowGrabber : public QThread, public QAVFScreenCapture::Grabber
{
public:
    WindowGrabber(QAVFScreenCapture &capture, CGWindowID wid) : m_capture(capture), m_wid(wid)
    {
        start();
    }

    ~WindowGrabber() override
    {
        quit();
        wait();
    }

private:
    void run() override
    {
        m_timer = std::make_unique<QTimer>();
        // should be deleted in this thread
        auto deleter = qScopeGuard([this]() { m_timer.reset(); });
        m_timer->setTimerType(Qt::PreciseTimer);

        QElapsedTimer elapsedTimer;
        qint64 lastFrameTime = 0;

        m_timer->callOnTimeout([&]() { grabWindow(elapsedTimer, lastFrameTime); });
        m_timer->start();

        exec();
    }

    void grabWindow(const QElapsedTimer &elapsedTimer, qint64 &lastFrameTime)
    {
        const auto frameRate = frameRateForWindow(m_wid);

        m_timer->setInterval(1000 / frameRate);

        auto imageRef = CGWindowListCreateImage(CGRectNull, kCGWindowListOptionIncludingWindow,
                                                m_wid, kCGWindowImageBoundsIgnoreFraming);
        if (!imageRef) {
            updateError(QScreenCapture::CaptureFailed,
                        QLatin1String("Cannot create image by window"));
            return;
        }

        auto imageDeleter = qScopeGuard([imageRef]() { CGImageRelease(imageRef); });

        if (CGImageGetBitsPerPixel(imageRef) != 32
            || CGImageGetPixelFormatInfo(imageRef) != kCGImagePixelFormatPacked
            || CGImageGetByteOrderInfo(imageRef) != kCGImageByteOrder32Little) {
            qWarning() << "Unexpected image format. PixelFormatInfo:"
                       << CGImageGetPixelFormatInfo(imageRef)
                       << "BitsPerPixel:" << CGImageGetBitsPerPixel(imageRef) << "AlphaInfo"
                       << CGImageGetAlphaInfo(imageRef)
                       << "ByteOrderInfo:" << CGImageGetByteOrderInfo(imageRef);

            updateError(QScreenCapture::WindowCapturingNotSupported,
                        QLatin1String("Not supported pixel format"));
            return;
        }

        QVideoFrameFormat format(QSize(CGImageGetWidth(imageRef), CGImageGetHeight(imageRef)),
                                 QVideoFrameFormat::Format_BGRA8888);
        format.setFrameRate(frameRate);

        QVideoFrame frame(new QCGImageVideoBuffer(imageRef), format);

        const auto endTime = elapsedTimer.nsecsElapsed() / 1000;

        frame.setStartTime(lastFrameTime);
        frame.setEndTime(endTime);

        m_capture.onNewFrame(frame);

        lastFrameTime = endTime;
    }

private:
    void updateError(QScreenCapture::Error error, const QString &description)
    {
        m_timer->setInterval(1000); // set some big interval in order not to overdo with the updates

        QMetaObject::invokeMethod(&m_capture, [&capture = m_capture, error, description]() {
            capture.updateError(error, description);
        });
    }

private:
    QAVFScreenCapture &m_capture;
    CGWindowID m_wid;
    std::unique_ptr<QTimer> m_timer;
};

class QAVFScreenCapture::ScreenGrabber : public QAVFScreenCapture::Grabber
{
public:
    ScreenGrabber(QAVFScreenCapture &capture, QScreen *screen, CGDirectDisplayID screenID,
                  std::unique_ptr<QFFmpeg::HWAccel> hwAccel)
    {
        m_captureSession = [[AVCaptureSession alloc] init];

        m_sampleBufferDelegate = [[QAVFSampleBufferDelegate alloc]
                initWithFrameHandler:[&capture](const QVideoFrame &frame) {
                    capture.onNewFrame(frame);
                }];

        m_videoDataOutput = [[AVCaptureVideoDataOutput alloc] init];

        NSDictionary *videoSettings = [NSDictionary
                dictionaryWithObjectsAndKeys:[NSNumber numberWithUnsignedInt:DefaultCVPixelFormat],
                                             kCVPixelBufferPixelFormatTypeKey, nil];

        [m_videoDataOutput setVideoSettings:videoSettings];
        [m_videoDataOutput setAlwaysDiscardsLateVideoFrames:true];

        // Configure video output
        m_dispatchQueue = dispatch_queue_create("vf_queue", nullptr);
        [m_videoDataOutput setSampleBufferDelegate:m_sampleBufferDelegate queue:m_dispatchQueue];

        [m_captureSession addOutput:m_videoDataOutput];

        [m_sampleBufferDelegate setHWAccel:std::move(hwAccel)];

        const auto frameRate = std::min(screen->refreshRate(), MaxFrameRate);
        [m_sampleBufferDelegate setVideoFormatFrameRate:frameRate];

        m_screenInput = [[AVCaptureScreenInput alloc] initWithDisplayID:screenID];
        [m_screenInput setMinFrameDuration:CMTimeMake(1, static_cast<int32_t>(frameRate))];
        [m_captureSession addInput:m_screenInput];

        [m_captureSession startRunning];
    }

    ~ScreenGrabber() override
    {
        if (m_captureSession)
            [m_captureSession stopRunning];

        if (m_dispatchQueue)
            dispatch_release(m_dispatchQueue);

        [m_sampleBufferDelegate release];
        [m_screenInput release];
        [m_videoDataOutput release];
        [m_captureSession release];
    }

private:
    AVCaptureSession *m_captureSession = nullptr;
    AVCaptureScreenInput *m_screenInput = nullptr;
    AVCaptureVideoDataOutput *m_videoDataOutput = nullptr;
    QAVFSampleBufferDelegate *m_sampleBufferDelegate = nullptr;
    dispatch_queue_t m_dispatchQueue = nullptr;
};

bool QAVFScreenCapture::initScreenCapture(QScreen *screen)
{
    const auto screenID = findDisplayByName(screen->name());

    if (screenID == kCGNullDirectDisplay) {
        updateError(QScreenCapture::InternalError,
                    QLatin1String("Screen exists but couldn't been found by name"));
        return false;
    }

    auto hwAccel = QFFmpeg::HWAccel::create(AV_HWDEVICE_TYPE_VIDEOTOOLBOX);

    if (!hwAccel) {
        updateError(QScreenCapture::CaptureFailed,
                    QLatin1String("Couldn't create videotoolbox hw acceleration"));
        return false;
    }

    hwAccel->createFramesContext(av_map_videotoolbox_format_to_pixfmt(DefaultCVPixelFormat),
                                 screen->size() * screen->devicePixelRatio());

    if (!hwAccel->hwFramesContextAsBuffer()) {
        updateError(QScreenCapture::CaptureFailed,
                    QLatin1String("Couldn't create hw frames context"));
        return false;
    }

    m_grabber = std::make_unique<ScreenGrabber>(*this, screen, screenID, std::move(hwAccel));
    return true;
}

bool QAVFScreenCapture::initWindowCapture(WId wid)
{
    const auto nativeWindowID = findNativeWindowID(wid);

    if (!nativeWindowID) {
        updateError(QScreenCapture::NotFound, QLatin1String("No native windows found"));
        return false;
    }

    m_grabber = std::make_unique<WindowGrabber>(*this, nativeWindowID);
    return true;
}

void QAVFScreenCapture::resetCapture()
{
    m_grabber.reset();
    m_format = {};
}

QT_END_NAMESPACE

#include "moc_qavfscreencapture_p.cpp"
