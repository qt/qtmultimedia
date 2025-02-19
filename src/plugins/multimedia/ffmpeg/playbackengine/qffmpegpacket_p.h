// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QFFMPEGPACKET_P_H
#define QFFMPEGPACKET_P_H

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

#include <QtFFmpegMediaPluginImpl/private/qffmpeg_p.h>
#include <QtFFmpegMediaPluginImpl/private/qffmpegplaybackutils_p.h>
#include <QtCore/qsharedpointer.h>

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

struct Packet
{
    struct Data : QSharedData
    {
        Data(const LoopOffset &offset, AVPacketUPtr p, quint64 sourceId)
            : loopOffset(offset), packet(std::move(p)), sourceId(sourceId)
        {
        }

        LoopOffset loopOffset;
        AVPacketUPtr packet;
        quint64 sourceId;
    };
    Packet() = default;
    Packet(const LoopOffset &offset, AVPacketUPtr p, quint64 sourceId)
        : d(new Data(offset, std::move(p), sourceId))
    {
    }

    bool isValid() const { return !!d; }
    AVPacket *avPacket() const { return d->packet.get(); }
    const LoopOffset &loopOffset() const { return d->loopOffset; }
    quint64 sourceId() const { return d->sourceId; }

private:
    QExplicitlySharedDataPointer<Data> d;
};

} // namespace QFFmpeg

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QFFmpeg::Packet)

#endif // QFFMPEGPACKET_P_H
