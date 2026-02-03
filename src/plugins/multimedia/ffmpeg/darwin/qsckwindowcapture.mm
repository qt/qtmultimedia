// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsckwindowcapture_p.h"

#include <QtCore/qthread.h>
#include <QtCore/private/qcore_mac_p.h>

#include <QtFFmpegMediaPluginImpl/private/qmacscreencapturekit_p.h>

#include <QtMultimedia/private/qcapturablewindow_p.h>
#include <QtMultimedia/private/qvideoframe_p.h>

#define AVMediaType XAVMediaType
extern "C" {
#include <libavutil/hwcontext_videotoolbox.h>
}
#undef AVMediaType

using namespace Qt::Literals::StringLiterals;

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

namespace {

[[nodiscard]] q23::expected<AVFScopedPointer<SCWindow>, QString> findScWindow(CGWindowID input)
{
    // Do a blocking enumeration of capturable items.
    q23::expected<QMacScreenCaptureKit::CapturableItems, QString> enumerateResult =
        QMacScreenCaptureKit::enumerateCapturableItems()
        .get();

    if (!enumerateResult)
        return q23::unexpected(enumerateResult.error());

    const std::vector<AVFScopedPointer<SCWindow>> &windows = enumerateResult->windows;
    auto it = std::find_if(
        windows.begin(),
        windows.end(),
        [&](const AVFScopedPointer<SCWindow> &item) {
            return item.data().windowID == input;
        });

    if (it == windows.end())
        return q23::unexpected(u"Window not found"_s);

    // AVFScopedPointer doesn't have shared-ptr semantics, force a reference increment.
    return AVFScopedPointer<SCWindow>{ [it->data() retain] };
}

void setupQMacScreenCaptureKitConnections(
    QSckWindowCapture &windowCapture,
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
        &windowCapture,
        [&windowCapture](int64_t, QVideoFrame videoFrame) {
            emit windowCapture.newVideoFrame(videoFrame);
        },
        Qt::DirectConnection);

    QObject::connect(
        &macScreenCaptureKit,
        &QMacScreenCaptureKit::newVideoFrameGenerated,
        &windowCapture,
        [&windowCapture](int64_t streamId, QVideoFrame newFrame) {
            windowCapture.onNewFrameFormatReceived(streamId, newFrame.surfaceFormat());
        },
        Qt::QueuedConnection);

    QObject::connect(
        &macScreenCaptureKit,
        &QMacScreenCaptureKit::streamStoppedWithError,
        &windowCapture,
        &QSckWindowCapture::onStreamStoppedWithErrorEvent,
        Qt::QueuedConnection);
}

} // Anonymous namespace

QSckWindowCapture::QSckWindowCapture() : QPlatformSurfaceCapture(WindowSource{})
{
}

bool QSckWindowCapture::setActiveInternal(bool active)
{
    if (active) {
        Q_ASSERT(!m_activeData);

        QCapturableWindow capturableWindow = source<WindowSource>();
        const QCapturableWindowPrivate *handle = QCapturableWindowPrivate::handle(capturableWindow);
        if (!handle) {
            qCWarning(qLcMacScreenCapture) << "Selected window is null";
            return false;
        }
        CGWindowID cgWindowId = static_cast<CGWindowID>(handle->id);

        // Find the associated SCWindow we can use.
        // This will trigger the system dialog for granting screen capture permissions.
        q23::expected<AVFScopedPointer<SCWindow>, QString> scWindowResult = findScWindow(cgWindowId);
        if (!scWindowResult) {
            qCWarning(qLcMacScreenCapture)
                << "Could not find associated SCWindow: "
                << scWindowResult.error();
            return false;
        }

        AVFScopedPointer<SCWindow> &scWindow = *scWindowResult;

        int64_t newStreamId = m_streamIdTracker++;

        // Start and wait for the stream. Blocking operation.
        using ResultType = q23::expected<std::unique_ptr<QMacScreenCaptureKit>, QString>;
        std::future<ResultType> streamResultFuture = QMacScreenCaptureKit::createStream(
            newStreamId,
            scWindow.data(),
            frameRate());
        ResultType streamResult = streamResultFuture.get();
        if (!streamResult) {
            qCWarning(qLcMacScreenCapture)
                << "Failed to start screen capture stream: "
                << streamResult.error();
            return false;
        }

        std::unique_ptr<QMacScreenCaptureKit> &macScreenCaptureKit = *streamResult;
        setupQMacScreenCaptureKitConnections(
            *this,
            *macScreenCaptureKit.get());

        auto newActiveData = std::make_unique<ActiveData>();
        newActiveData->macScreenCaptureKit = std::move(macScreenCaptureKit);
        newActiveData->scWindow = std::move(scWindow);
        newActiveData->streamId = newStreamId;
        m_activeData = std::move(newActiveData);

        return true;

    } else {
        m_activeData.reset();
    }

    return true;
}

void QSckWindowCapture::onNewFrameFormatReceived(
    int64_t incomingStreamId,
    QVideoFrameFormat const &format)
{
    Q_ASSERT(thread()->isCurrentThread());

    std::optional<int64_t> activeStreamIdOpt = activeStreamId();
    if (activeStreamIdOpt.has_value() && *activeStreamIdOpt == incomingStreamId)
        m_videoFrameFormat = format;
}

void QSckWindowCapture::onStreamStoppedWithErrorEvent(
    int64_t incomingStreamId,
    const QString &err)
{
    Q_ASSERT(thread()->isCurrentThread());

    qCDebug(qLcMacScreenCapture)
        << "Stream with ID "
        << incomingStreamId
        << " stopped with error: "
        << err;

    std::optional<int64_t> activeStreamIdOpt = activeStreamId();
    if (!activeStreamIdOpt || *activeStreamIdOpt != incomingStreamId)
        return;

    // Possible improvement may be to propagate signal up to QVideoSource
    // that we are no longer active.
    m_activeData.reset();
    m_videoFrameFormat.reset();

    QPlatformSurfaceCapture::updateError(
        Error::CaptureFailed,
        u"The capture stream was closed by the system"_s);
}

QVideoFrameFormat QSckWindowCapture::frameFormat() const
{
    Q_ASSERT(thread()->isCurrentThread());

    if (m_videoFrameFormat)
        return *m_videoFrameFormat;
    return {};
}

std::optional<int> QSckWindowCapture::ffmpegHWPixelFormat() const
{
    Q_ASSERT(thread()->isCurrentThread());
    return AV_PIX_FMT_VIDEOTOOLBOX;
}

std::unique_ptr<QPlatformSurfaceCapture> makeQSckWindowCapture()
{
    return std::make_unique<QSckWindowCapture>();
}

} // namespace QFFmpeg

QT_END_NAMESPACE

#include "moc_qsckwindowcapture_p.cpp"
