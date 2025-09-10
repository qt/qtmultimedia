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
#include <QtFFmpegMediaPluginImpl/private/qffmpegplaybackengineobject_p.h>
#include <QtFFmpegMediaPluginImpl/private/qffmpegframe_p.h>
#include <QtFFmpegMediaPluginImpl/private/qffmpegpacket_p.h>
#include <QtFFmpegMediaPluginImpl/private/qffmpegplaybackutils_p.h>
#include <QtMultimedia/private/qplatformmediaplayer_p.h>

#include <QtCore/qqueue.h>

#include <optional>

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

class StreamDecoder : public PlaybackEngineObject
{
    Q_OBJECT
public:
    StreamDecoder(const CodecContext &codecContext, TrackPosition absSeekPos);

    ~StreamDecoder() override;

    QPlatformMediaPlayer::TrackType trackType() const;

    // Maximum number of frames that we are allowed to keep in render queue
    static qint32 maxQueueSize(QPlatformMediaPlayer::TrackType type);

public slots:

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

    void receiveAVFrames(bool flushPacket = false);

private:
    CodecContext m_codecContext;
    TrackPosition m_absSeekPos = TrackPosition(0);
    const QPlatformMediaPlayer::TrackType m_trackType;

    qint32 m_pendingFramesCount = 0;

    LoopOffset m_offset;

    QQueue<Packet> m_packets;
};

} // namespace QFFmpeg

QT_END_NAMESPACE

#endif // QFFMPEGSTREAMDECODER_P_H
