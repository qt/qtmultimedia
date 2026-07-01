// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsckscreencapture_p.h"

#include <QtCore/qthread.h>

#include <QtFFmpegMediaPluginImpl/private/qffmpegmediacapturesession_p.h>
#include <QtFFmpegMediaPluginImpl/private/qmacscreencapturekit_p.h>

#include <QtMultimedia/private/qmultimedia_ranges_p.h>

#include <QtGui/qscreen.h>

#define AVMediaType XAVMediaType
extern "C" {
#include <libavutil/hwcontext_videotoolbox.h>
}
#undef AVMediaType

#import <AppKit/NSScreen.h>

#include <algorithm>

namespace ranges = QtMultimediaPrivate::ranges;
using namespace Qt::Literals::StringLiterals;

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

[[nodiscard]] static CGDirectDisplayID displayIDForScreen(NSScreen *screen)
{
#if QT_MACOS_PLATFORM_SDK_EQUAL_OR_ABOVE(260000)
    if (@available(macOS 26.0, *))
        return screen.CGDirectDisplayID;
#endif

    NSNumber *screenNumber = screen.deviceDescription[@"NSScreenNumber"];
    return static_cast<CGDirectDisplayID>(screenNumber.unsignedIntValue);
}

[[nodiscard]] static q23::expected<AVFScopedPointer<SCDisplay>, QString> findScDisplay(CGDirectDisplayID input)
{
    // Do a blocking enumeration of capturable items.
    q23::expected<QMacScreenCaptureKit::CapturableItems, QString> enumerateResult =
        QMacScreenCaptureKit::enumerateCapturableItems()
        .get();

    if (!enumerateResult)
        return q23::unexpected{ std::move(enumerateResult.error()) };

    const std::vector<AVFScopedPointer<SCDisplay>> &displays = enumerateResult->displays;
    auto it = ranges::find_if(
        displays,
        [&](const AVFScopedPointer<SCDisplay> &item) {
            return item.data().displayID == input;
        });
    if (it == displays.end())
        return q23::unexpected(u"Display not found"_s);

    // AVFScopedPointer doesn't have shared-ptr semantics, force a reference increment.
    return AVFScopedPointer<SCDisplay>{ [it->data() retain] };
}

static void setupQMacScreenCaptureKitConnections(
    QSckScreenCapture &screenCapture,
    const QMacScreenCaptureKit &macScreenCaptureKit)
{
    // Direct connection so application developers can respond to frame directly
    // from background thread.
    // Remaining frames are always flushed whenever we go inactive, as a result
    // of QMacScreenCaptureKit doing so in the destructor. Because we flush,
    // we trust the application developer to not block the background thread.
    QObject::connect(
        &macScreenCaptureKit,
        &QMacScreenCaptureKit::newVideoFrameGenerated,
        &screenCapture,
        [&screenCapture](QMacScreenCaptureKit::StreamId, QVideoFrame videoFrame) {
            emit screenCapture.newVideoFrame(videoFrame);
        },
        Qt::DirectConnection);

    QObject::connect(
        &macScreenCaptureKit,
        &QMacScreenCaptureKit::newVideoFrameGenerated,
        &screenCapture,
        [&screenCapture](QMacScreenCaptureKit::StreamId streamId, QVideoFrame newFrame) {
            screenCapture.onNewFrameFormatReceived(streamId, newFrame.surfaceFormat());
        },
        Qt::QueuedConnection);

    QObject::connect(
        &macScreenCaptureKit,
        &QMacScreenCaptureKit::streamStoppedWithError,
        &screenCapture,
        &QSckScreenCapture::onStreamStoppedWithErrorEvent,
        Qt::QueuedConnection);
}


QSckScreenCapture::QSckScreenCapture() : QPlatformSurfaceCapture(ScreenSource{})
{
}

void QSckScreenCapture::setCaptureSession(QPlatformMediaCaptureSession *sessionIn)
{
    if (!sessionIn) {
        m_session = nullptr;
        return;
    }

    auto *session = qobject_cast<QFFmpegMediaCaptureSession *>(sessionIn);
    Q_ASSERT(session);
    m_session = session;
}

bool QSckScreenCapture::setActiveInternal(bool active)
{
    struct ErrorPair {
        QPlatformSurfaceCapture::Error err;
        QString msg;
    };

    using TryStartResult = q23::expected<std::unique_ptr<ActiveData>, ErrorPair>;

    auto tryStartStream = [&]() -> TryStartResult {
        ScreenSource screen = QPlatformSurfaceCapture::source<ScreenSource>();
        if (!QPlatformSurfaceCapture::checkScreenWithError(screen)) {
            return q23::unexpected{ ErrorPair {
               QPlatformSurfaceCapture::Error::NotFound,
               u"Could not find selected screen"_s } };
        }

        auto *cocoaScreen = screen->nativeInterface<QNativeInterface::QCocoaScreen>();
        if (!cocoaScreen) {
            qCWarning(qLcMacScreenCapture)
                << "Failed to grab QNativeInterface::QCocoaScreen from QScreen";
            return q23::unexpected{ ErrorPair{
                QPlatformSurfaceCapture::Error::CaptureFailed,
                u"Failed to start stream due to unknown issue"_s } };
        }

        CGDirectDisplayID cgDisplayId = displayIDForScreen(cocoaScreen->nativeScreen());

        q23::expected<AVFScopedPointer<SCDisplay>, QString> scDisplayResult = findScDisplay(cgDisplayId);
        if (!scDisplayResult) {
            qCWarning(qLcMacScreenCapture)
                << "Could not find associated SCDisplay:"
                << scDisplayResult.error();
            return q23::unexpected{ ErrorPair {
                QPlatformSurfaceCapture::Error::NotFound,
                u"Backend was unable to find selected QScreen"_s } };
        }

        QMacScreenCaptureKit::StreamId newStreamId = allocateStreamId();

        AVFScopedPointer<SCDisplay> &scDisplay = *scDisplayResult;

        auto setupConnections = [&](QMacScreenCaptureKit &newObject) {
            setupQMacScreenCaptureKitConnections(*this, newObject);
        };

        if (m_session)
            m_session->onSourceActivating(*this);

        // Start and wait for the stream to start. Blocking operation.
        using ResultType = q23::expected<std::unique_ptr<QMacScreenCaptureKit>, QString>;
        std::future<ResultType> streamResultFuture = QMacScreenCaptureKit::createStreamFromDisplay(
            newStreamId,
            scDisplay.data(),
            frameRate(),
            setupConnections);
        ResultType streamResult = streamResultFuture.get();
        if (!streamResult) {
            if (m_session)
                m_session->onSourceActivationFailure(*this);

            qCWarning(qLcMacScreenCapture)
                << "Failed to start screen capture stream: "
                << streamResult.error();
            return q23::unexpected{ ErrorPair{
               QPlatformSurfaceCapture::Error::CaptureFailed,
               u"Failed to start stream due to unknown issue"_s } };
        }

        std::unique_ptr<QMacScreenCaptureKit> &macScreenCaptureKit = *streamResult;

        auto newActiveData = std::make_unique<ActiveData>();
        newActiveData->macScreenCaptureKit = std::move(macScreenCaptureKit);
        newActiveData->scDisplay = std::move(scDisplay);
        newActiveData->streamId = newStreamId;
        return newActiveData;
    };

    if (active) {
        Q_ASSERT(!m_activeData);

        TryStartResult result = tryStartStream();
        if (!result) {
            const ErrorPair &error = result.error();
            QPlatformSurfaceCapture::updateError(error.err, error.msg);
            return false;
        }

        m_activeData = std::move(*result);
    } else {
        m_activeData.reset();
    }

    return true;
}

void QSckScreenCapture::onNewFrameFormatReceived(
    QMacScreenCaptureKit::StreamId incomingStreamId,
    QVideoFrameFormat const &format)
{
    Q_ASSERT(thread()->isCurrentThread());
    if (activeStreamId() == incomingStreamId)
        m_videoFrameFormat = format;
}

void QSckScreenCapture::onStreamStoppedWithErrorEvent(
    QMacScreenCaptureKit::StreamId incomingStreamId,
    const QString &err)
{
    Q_ASSERT(thread()->isCurrentThread());

    qCDebug(qLcMacScreenCapture)
        << "Stream with ID"
        << static_cast<int64_t>(incomingStreamId)
        << "stopped with error:"
        << err;

    if (activeStreamId() != incomingStreamId)
        return;

    // Possible improvement may be to propagate signal up to QVideoSource
    // that we are no longer active.
    m_activeData.reset();
    m_videoFrameFormat.reset();

    QPlatformSurfaceCapture::updateError(
        Error::CaptureFailed,
        u"The capture stream was closed by the system"_s);
}

QVideoFrameFormat QSckScreenCapture::frameFormat() const
{
    Q_ASSERT(thread()->isCurrentThread());

    if (m_videoFrameFormat)
        return *m_videoFrameFormat;
    return {};
}

std::optional<int> QSckScreenCapture::ffmpegHWPixelFormat() const
{
    Q_ASSERT(thread()->isCurrentThread());
    return AV_PIX_FMT_VIDEOTOOLBOX;
}

std::unique_ptr<QPlatformSurfaceCapture> makeQSckScreenCapture()
{
    return std::make_unique<QSckScreenCapture>();
}

} // namespace QFFmpeg

QT_END_NAMESPACE

#include "moc_qsckscreencapture_p.cpp"
