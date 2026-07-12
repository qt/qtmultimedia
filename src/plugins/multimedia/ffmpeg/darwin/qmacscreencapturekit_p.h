// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QMACSCREENCAPTUREKIT_P_H
#define QMACSCREENCAPTUREKIT_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qobject.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/private/qexpected_p.h>

#include <QtMultimedia/qvideoframe.h>
#include <QtMultimedia/private/qavfcamerautility_p.h>

#import <ScreenCaptureKit/ScreenCaptureKit.h>

#include <functional>
#include <future>
#include <memory>
#include <optional>
#include <vector>

// Error-handling for on-going stream
@interface QT_MANGLE_NAMESPACE(QMacScreenCaptureStreamDelegate) : NSObject <SCStreamDelegate>
@end

// Receives frame callbacks
@interface QT_MANGLE_NAMESPACE(QMacScreenCaptureStreamOutput) : NSObject <SCStreamOutput>
@end

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

Q_DECLARE_LOGGING_CATEGORY(qLcMacScreenCapture);

using QMacScreenCaptureStreamDelegate = QT_MANGLE_NAMESPACE(QMacScreenCaptureStreamDelegate);
using QMacScreenCaptureStreamOutput = QT_MANGLE_NAMESPACE(QMacScreenCaptureStreamOutput);

// Helper class for managing a ScreenCaptureKit stream.
//
// TODO: This class has very few FFmpeg specific components. Move this out of FFmpeg media backend
// in the future.
//
// TODO: Use this backend for QScreenCapture as well.
class QMacScreenCaptureKit : public QObject {
    Q_OBJECT

public:
    enum class StreamId : int64_t{};

    // TODO: May become a runtime parameter in the future.
    // For now we only support BGRA32.
    static constexpr FourCharCode cvPixelFormat = kCVPixelFormatType_32BGRA;
    static constexpr QVideoFrameFormat::PixelFormat pixelFormat =
        QVideoFrameFormat::PixelFormat::Format_BGRA8888;
    // How many frames can be queued up on the background thread before we start dropping frames
    // ScreenCaptureKit default is 3.
    // https://developer.apple.com/documentation/screencapturekit/scstreamconfiguration/queuedepth?language=objc
    static constexpr int queueDepth = 3;
    [[nodiscard]] static CFStringRef cgColorSpace() { return kCGColorSpaceSRGB; }
    static constexpr QVideoFrameFormat::ColorSpace colorSpace =
        QVideoFrameFormat::ColorSpace::ColorSpace_BT709;
    static constexpr QVideoFrameFormat::ColorRange colorRange =
        QVideoFrameFormat::ColorRange::ColorRange_Full;
    static constexpr QVideoFrameFormat::ColorTransfer colorTransfer =
        QVideoFrameFormat::ColorTransfer::ColorTransfer_BT709;

    struct CapturableItems {
        std::vector<AVFScopedPointer<SCDisplay>> displays;
        std::vector<AVFScopedPointer<SCWindow>> windows;
    };

    QMacScreenCaptureKit() = default;
    ~QMacScreenCaptureKit();
    QMacScreenCaptureKit(const QMacScreenCaptureKit &) = delete;
    QMacScreenCaptureKit &operator=(const QMacScreenCaptureKit &) = delete;
    QMacScreenCaptureKit(QMacScreenCaptureKit &&other) noexcept = delete;
    QMacScreenCaptureKit &operator=(QMacScreenCaptureKit &&other) = delete;

    [[nodiscard]] StreamId streamId() const noexcept { return m_streamId; }

    [[nodiscard]] static AVFScopedPointer<SCStreamConfiguration> createStreamConfig(
        QSize resolutionPx,
        std::optional<qreal> frameRate);

    [[nodiscard]] static std::future<q23::expected<CapturableItems, QString>> enumerateCapturableItems();

    [[nodiscard]] static std::future<q23::expected<std::unique_ptr<QMacScreenCaptureKit>, QString>>
    createStreamFromFilter(
        StreamId streamId,
        SCContentFilter *,
        QSize resolutionPx,
        std::optional<qreal> frameRate,
        std::function<void(QMacScreenCaptureKit&)> const &connectionSetup);

    [[nodiscard]] static std::future<q23::expected<std::unique_ptr<QMacScreenCaptureKit>, QString>>
    createStreamFromWindow(
        StreamId streamId,
        SCWindow *,
        std::optional<qreal> frameRate,
        std::function<void(QMacScreenCaptureKit&)> const &connectionSetup);

    // Called from background thread when stream needs to be configured with new
    // output resolution. Don't call directly.
    void updateStream(QSize resolutionPx);

signals:
    void newVideoFrameGenerated(StreamId streamId, QVideoFrame);
    // This is commonly signaled if the user stops the stream by
    // interacting with the system UI, or the stream has stopped
    // for unknown reasons.
    // Message is not suitable for forwarding to UI.
    void streamStoppedWithError(StreamId streamId, QString);

private:
    StreamId m_streamId = {};

    // A copy of the frameRate when this class was created. Does not
    // change after stream has started.
    std::optional<qreal> m_frameRate;

    AVFScopedPointer<SCStream> m_stream;
    AVFScopedPointer<dispatch_queue_t> m_dispatchQueue;
    AVFScopedPointer<QMacScreenCaptureStreamDelegate> m_streamDelegate;
    AVFScopedPointer<QMacScreenCaptureStreamOutput> m_streamOutput;

    static void startStreamReconfigure(
        SCStream *scStream,
        QSize resolutionPx,
        std::optional<qreal> frameRate);
};

}

QT_END_NAMESPACE

#endif // QMACSCREENCAPTUREKIT_P_H
