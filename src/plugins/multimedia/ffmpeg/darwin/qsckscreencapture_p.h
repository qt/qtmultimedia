// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSCKSCREENCAPTURE_P_H
#define QSCKSCREENCAPTURE_P_H

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

#include <QtMultimedia/private/qplatformsurfacecapture_p.h>

#include <QtFFmpegMediaPluginImpl/private/qmacscreencapturekit_p.h>

QT_BEGIN_NAMESPACE

class QFFmpegMediaCaptureSession;

namespace QFFmpeg {

class QSckScreenCapture : public QPlatformSurfaceCapture
{
    Q_OBJECT

public:
    explicit QSckScreenCapture();
    ~QSckScreenCapture() override = default;

    void setCaptureSession(QPlatformMediaCaptureSession *) override;

    QVideoFrameFormat frameFormat() const override;

    std::optional<int> ffmpegHWPixelFormat() const override;

    void onNewFrameFormatReceived(QMacScreenCaptureKit::StreamId streamId, const QVideoFrameFormat &);

    void onStreamStoppedWithErrorEvent(QMacScreenCaptureKit::StreamId streamId, const QString &);

protected:
    bool setActiveInternal(bool active) override;

private:
    QFFmpegMediaCaptureSession *m_session = nullptr;

    // Tracks the next ID for establishing stream.
    // Having stream IDs helps us track whether we should
    // ignore events from lingering stream callbacks.
    int64_t m_streamIdAllocator = 0;
    [[nodiscard]] QMacScreenCaptureKit::StreamId allocateStreamId() {
        return QMacScreenCaptureKit::StreamId { m_streamIdAllocator++ };
    }

    struct ActiveData {
        QMacScreenCaptureKit::StreamId streamId = {};
        AVFScopedPointer<SCDisplay> scDisplay;
        std::unique_ptr<QMacScreenCaptureKit> macScreenCaptureKit;
    };
    // Is allowed to be null when we have no on-going stream.
    std::unique_ptr<ActiveData> m_activeData;

    std::optional<QVideoFrameFormat> m_videoFrameFormat;

    [[nodiscard]] std::optional<QMacScreenCaptureKit::StreamId> activeStreamId() const noexcept {
        if (m_activeData)
            return m_activeData->streamId;
        return std::nullopt;
    }

};

} // namespace QFFmpeg

QT_END_NAMESPACE

#endif // QSCKSCREENCAPTURE_P_H
