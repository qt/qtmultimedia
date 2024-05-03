// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QFFMPEGMUXER_P_H
#define QFFMPEGMUXER_P_H

#include "qffmpegthread_p.h"
#include "qffmpeg_p.h"
#include <queue>

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

class RecordingEngine;

class Muxer : public ConsumerThread
{
public:
    Muxer(RecordingEngine *encoder);

    void addPacket(AVPacketUPtr packet);

private:
    AVPacketUPtr takePacket();

    void init() override;
    void cleanup() override;
    bool hasData() const override;
    void processOne() override;

private:
    std::queue<AVPacketUPtr> m_packetQueue;

    RecordingEngine *m_encoder;
};

} // namespace QFFmpeg

QT_END_NAMESPACE

#endif
