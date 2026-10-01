// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsckscreencapture_p.h"

#include <QtCore/qthread.h>

#include <QtFFmpegMediaPluginImpl/private/qffmpegmediacapturesession_p.h>
#include <QtFFmpegMediaPluginImpl/private/qmacscreencapturekit_p.h>

#include <QtMultimedia/private/qmultimedia_ranges_p.h>

#include <QtGui/qguiapplication.h>
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

// Finds the SCDisplay to capture for the given, non-null QScreen.
[[nodiscard]] static auto findScDisplayForScreen(QScreen &screen)
    -> q23::expected<AVFScopedPointer<SCDisplay>, QPlatformSurfaceCapture::ErrorDetails>
{
    using Error = QPlatformSurfaceCapture::Error;
    using ErrorDetails = QPlatformSurfaceCapture::ErrorDetails;

    auto *cocoaScreen = screen.nativeInterface<QNativeInterface::QCocoaScreen>();
    if (!cocoaScreen) {
        qCWarning(qLcMacScreenCapture)
            << "Failed to grab QNativeInterface::QCocoaScreen from QScreen";
        return q23::unexpected{ ErrorDetails{
            Error::CaptureFailed,
            u"Screen capture could not be started."_s } };
    }

    CGDirectDisplayID cgDisplayId = displayIDForScreen(cocoaScreen->nativeScreen());

    q23::expected<AVFScopedPointer<SCDisplay>, QString> scDisplayResult = findScDisplay(cgDisplayId);
    if (!scDisplayResult) {
        qCWarning(qLcMacScreenCapture)
            << "Could not find associated SCDisplay:"
            << scDisplayResult.error();
        return q23::unexpected{ ErrorDetails{
            Error::NotFound,
            u"The selected screen could not be found."_s } };
    }

    return std::move(*scDisplayResult);
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
            Q_EMIT screenCapture.newVideoFrame(videoFrame);
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
    using TryStartResult = q23::expected<std::unique_ptr<ActiveData>, ErrorDetails>;

    auto tryStartStream = [&]() -> TryStartResult {
        ScreenSource screen = QPlatformSurfaceCapture::source<ScreenSource>();
        if (!QPlatformSurfaceCapture::checkScreenWithError(screen)) {
            return q23::unexpected{ ErrorDetails{
               QPlatformSurfaceCapture::Error::NotFound,
               u"The selected screen could not be found. It may have been disconnected."_s } };
        }

        q23::expected<AVFScopedPointer<SCDisplay>, ErrorDetails> scDisplayResult =
            findScDisplayForScreen(*screen);
        if (!scDisplayResult)
            return q23::unexpected{ std::move(scDisplayResult.error()) };

        QMacScreenCaptureKit::StreamId newStreamId = allocateStreamId();

        AVFScopedPointer<SCDisplay> &scDisplay = *scDisplayResult;

        if (m_session)
            m_session->onSourceActivating(*this);

        // Start and wait for the stream to start. Blocking operation.
        QMacScreenCaptureKit::CreateStreamInfo createInfo = {};
        createInfo.streamId = newStreamId;
        createInfo.connectionSetupFn = [&](QMacScreenCaptureKit &newObject) {
            setupQMacScreenCaptureKitConnections(*this, newObject);
        };
        createInfo.streamSettings.frameRate = frameRate();
        createInfo.streamSettings.overrideIgnoreCursor = ignoreCursor();
        using ResultType = q23::expected<std::unique_ptr<QMacScreenCaptureKit>, QString>;
        std::future<ResultType> streamResultFuture = QMacScreenCaptureKit::createStreamFromDisplay(
            createInfo,
            scDisplay.data());
        ResultType streamResult = streamResultFuture.get();
        if (!streamResult) {
            if (m_session)
                m_session->onSourceActivationFailure(*this);

            qCWarning(qLcMacScreenCapture)
                << "Failed to start screen capture stream: "
                << streamResult.error();
            return q23::unexpected{ ErrorDetails{
               QPlatformSurfaceCapture::Error::CaptureFailed,
               u"Screen capture could not be started."_s } };
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
            const ErrorDetails &error = result.error();
            QPlatformSurfaceCapture::updateError(error.error, error.description);
            return false;
        }

        m_activeData = std::move(*result);
    } else {
        m_activeData.reset();
    }

    return true;
}

std::optional<QPlatformSurfaceCapture::SourceSwitchResult>
QSckScreenCapture::switchActiveSource(const Source &newSource)
{
    Q_ASSERT(m_activeData);

    // A null screen means the primary screen.
    QScreen *screen = std::get<ScreenSource>(newSource);
    if (!screen)
        screen = QGuiApplication::primaryScreen();
    if (!screen)
        return SourceSwitchResult{ q23::unexpected{ ErrorDetails{
            Error::NotFound,
            u"The selected screen could not be found. It may have been disconnected."_s } } };

    q23::expected<AVFScopedPointer<SCDisplay>, ErrorDetails> scDisplayResult =
        findScDisplayForScreen(*screen);
    if (!scDisplayResult)
        return SourceSwitchResult{ q23::unexpected{ std::move(scDisplayResult.error()) } };

    std::future<q23::expected<void, QString>> switchFuture =
        m_activeData->macScreenCaptureKit->switchToDisplay(scDisplayResult->data());

    const q23::expected<void, QString> result = switchFuture.get();
    if (!result) {
        qCWarning(qLcMacScreenCapture)
            << "Failed to switch screen capture stream to new screen:"
            << result.error();
        return SourceSwitchResult{ q23::unexpected{ ErrorDetails{
            Error::CaptureFailed,
            u"Could not switch capture to the selected screen."_s } } };
    }

    m_activeData->scDisplay = std::move(*scDisplayResult);
    return SourceSwitchResult{};
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
