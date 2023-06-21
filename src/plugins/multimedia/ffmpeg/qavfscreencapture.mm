// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qavfscreencapture_p.h"
#include "qavfsamplebufferdelegate_p.h"
#include "qffmpegsurfacecapturethread_p.h"

#include <qscreen.h>

#define AVMediaType XAVMediaType
#include "qffmpeghwaccel_p.h"

extern "C" {
#include <libavutil/hwcontext_videotoolbox.h>
#include <libavutil/hwcontext.h>
}
#undef AVMediaType

#include <AppKit/NSScreen.h>

#include <dispatch/dispatch.h>

namespace {

const auto DefaultCVPixelFormat = kCVPixelFormatType_32BGRA;

CGDirectDisplayID findDisplayByName(const QString &name)
{
    for (NSScreen *screen in NSScreen.screens) {
        if (name == QString::fromNSString(screen.localizedName))
            return [screen.deviceDescription[@"NSScreenNumber"] unsignedIntValue];
    }
    return kCGNullDirectDisplay;
}
}

QT_BEGIN_NAMESPACE

QAVFScreenCapture::QAVFScreenCapture() : QPlatformSurfaceCapture(ScreenSource{})
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
            updateError(CaptureFailed, QLatin1String("Permissions denied"));
            return false;
        }

        auto screen = source<ScreenSource>();

        if (!checkScreenWithError(screen))
            return false;

        return initScreenCapture(screen);
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

QVideoFrameFormat QAVFScreenCapture::frameFormat() const
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

class QAVFScreenCapture::Grabber
{
public:
    Grabber(QAVFScreenCapture &capture, QScreen *screen, CGDirectDisplayID screenID,
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

        const auto frameRate = std::min(screen->refreshRate(), MaxScreenCaptureFrameRate);
        [m_sampleBufferDelegate setVideoFormatFrameRate:frameRate];

        m_screenInput = [[AVCaptureScreenInput alloc] initWithDisplayID:screenID];
        [m_screenInput setMinFrameDuration:CMTimeMake(1, static_cast<int32_t>(frameRate))];
        [m_captureSession addInput:m_screenInput];

        [m_captureSession startRunning];
    }

    ~Grabber()
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
        updateError(InternalError, QLatin1String("Screen exists but couldn't been found by name"));
        return false;
    }

    auto hwAccel = QFFmpeg::HWAccel::create(AV_HWDEVICE_TYPE_VIDEOTOOLBOX);

    if (!hwAccel) {
        updateError(CaptureFailed, QLatin1String("Couldn't create videotoolbox hw acceleration"));
        return false;
    }

    hwAccel->createFramesContext(av_map_videotoolbox_format_to_pixfmt(DefaultCVPixelFormat),
                                 screen->size() * screen->devicePixelRatio());

    if (!hwAccel->hwFramesContextAsBuffer()) {
        updateError(CaptureFailed, QLatin1String("Couldn't create hw frames context"));
        return false;
    }

    m_grabber = std::make_unique<Grabber>(*this, screen, screenID, std::move(hwAccel));
    return true;
}

void QAVFScreenCapture::resetCapture()
{
    m_grabber.reset();
    m_format = {};
}

QT_END_NAMESPACE

#include "moc_qavfscreencapture_p.cpp"
