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

#include "qffmpeg_p.h"
#include "QtCore/qsharedpointer.h"

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

struct Packet
{
    struct Data
    {
        Data(AVPacketUPtr p) : packet(std::move(p)) { }

        QAtomicInt ref;
        AVPacketUPtr packet;
    };
    Packet() = default;
    Packet(AVPacketUPtr p) : d(new Data(std::move(p))) { }

    bool isValid() const { return !!d; }
    AVPacket *avPacket() const { return d->packet.get(); }

private:
    QExplicitlySharedDataPointer<Data> d;
};

} // namespace QFFmpeg

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QFFmpeg::Packet)

#endif // QFFMPEGPACKET_P_H
