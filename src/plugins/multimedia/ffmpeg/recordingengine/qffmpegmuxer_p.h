// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QFFMPEGMUXER_P_H
#define QFFMPEGMUXER_P_H

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

#include <QtFFmpegMediaPluginImpl/private/qffmpegthread_p.h>
#include <QtFFmpegMediaPluginImpl/private/qffmpeg_p.h>
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

    bool init() override;
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
