// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QFFMPEGFRAMERATEADAPTER_P_H
#define QFFMPEGFRAMERATEADAPTER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtMultimedia/qvideoframe.h>
#include <QtCore/qlist.h>
#include <QtFFmpegMediaPluginImpl/private/qffmpegrecordingengineutils_p.h>

#include <optional>

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

// Adapts an incoming video stream to a target frame rate by dropping (source faster)
// or duplicating (source slower) frames. Output PTS is slot-aligned and never adjusted
// backward relative to the encoded frame's capture time.
class FrameRateAdapter
{
public:
    FrameRateAdapter() = default;

    void setRates(std::optional<double> sourceRate, double targetRate);

    [[nodiscard]] bool isActive() const { return m_slotDuration > 0; }
    [[nodiscard]] qint64 frameDuration() const { return m_slotDuration; }

    // Process one frame. Returns frames to be encoded.
    // Returns an empty queue when the frame was buffered as a pending candidate
    // (a later frame will trigger its emission with a forward-adjusted PTS).
    // adjustTimeBase is stored alongside the pending frame and forwarded to
    // the first output frame when the pending candidate is eventually emitted.
    std::vector<FrameInfo> adapt(const QVideoFrame &frame, bool adjustTimeBase = false);

    std::optional<FrameInfo> flush();

    // Clear all internal state (call on pause/seek).
    void reset();

private:
    [[nodiscard]] FrameInfo emitPendingFrame(bool keepPending = true);

    qint64 m_slotDuration{ 0 }; // µs per encoder frame; 0 = inactive
    qint64 m_nextSlotTime{ -1 }; // next slot boundary in µs; -1 = uninitialized
    qint64 m_slotMatchEpsilon{ 0 };

    std::optional<FrameInfo> m_pendingFrame;
    std::optional<qint64> m_sourceDuration{ 0 };
    bool m_pendingNeedsFlush{ false };
};

} // namespace QFFmpeg

QT_END_NAMESPACE

#endif
