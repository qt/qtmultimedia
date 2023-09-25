// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QFFMPEGSTREAMDECODER_P_H
#define QFFMPEGSTREAMDECODER_P_H

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
#include "playbackengine/qffmpegplaybackengineobject_p.h"
#include "playbackengine/qffmpegframe_p.h"
#include "playbackengine/qffmpegpacket_p.h"
#include "playbackengine/qffmpegpositionwithoffset_p.h"
#include "private/qplatformmediaplayer_p.h"

#include <optional>

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

class StreamDecoder : public PlaybackEngineObject
{
    Q_OBJECT
public:
    StreamDecoder(const Codec &codec, qint64 absSeekPos);

    ~StreamDecoder();

    QPlatformMediaPlayer::TrackType trackType() const;

public slots:
    void setInitialPosition(TimePoint tp, qint64 trackPos);

    void decode(Packet);

    void onFinalPacketReceived();

    void onFrameProcessed(Frame frame);

signals:
    void requestHandleFrame(Frame frame);

    void packetProcessed(Packet);

protected:
    bool canDoNextStep() const override;

    void doNextStep() override;

private:
    void decodeMedia(Packet);

    void decodeSubtitle(Packet);

    void onFrameFound(Frame frame);

    int sendAVPacket(Packet);

    void receiveAVFrames();

private:
    Codec m_codec;
    qint64 m_absSeekPos = 0;
    const QPlatformMediaPlayer::TrackType m_trackType;

    qint32 m_pendingFramesCount = 0;

    LoopOffset m_offset;

    QQueue<Packet> m_packets;
};

} // namespace QFFmpeg

QT_END_NAMESPACE

#endif // QFFMPEGSTREAMDECODER_P_H
