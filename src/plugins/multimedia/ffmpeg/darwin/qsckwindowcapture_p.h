// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSCKWINDOWCAPTURE_H
#define QSCKWINDOWCAPTURE_H

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

#include <memory>

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

class QSckWindowCapture : public QPlatformSurfaceCapture
{
    Q_OBJECT

public:
    explicit QSckWindowCapture();
    ~QSckWindowCapture() override = default;

    QVideoFrameFormat frameFormat() const override;

    std::optional<int> ffmpegHWPixelFormat() const override;

    void onNewFrameFormatReceived(int64_t streamId, const QVideoFrameFormat &);

    void onStreamStoppedWithErrorEvent(int64_t streamId, const QString &);

protected:
    bool setActiveInternal(bool active) override;

private:
    // Tracks the next ID for establishing stream.
    // Having stream IDs helps us track whether we should
    // ignore events from lingering stream callbacks.
    int64_t m_streamIdTracker = 0;

    struct ActiveData {
        int64_t streamId = -1;
        AVFScopedPointer<SCWindow> scWindow;
        std::unique_ptr<QMacScreenCaptureKit> macScreenCaptureKit;
    };
    // Is allowed to be null when we have no on-going stream.
    std::unique_ptr<ActiveData> m_activeData;

    std::optional<QVideoFrameFormat> m_videoFrameFormat;

    [[nodiscard]] std::optional<int64_t> activeStreamId() const noexcept {
        if (m_activeData)
            return m_activeData->streamId;
        return std::nullopt;
    }
};

} // namespace QFFmpeg

QT_END_NAMESPACE

#endif // QSCKWINDOWCAPTURE_H
