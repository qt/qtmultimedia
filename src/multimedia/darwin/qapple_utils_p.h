// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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

#ifndef QAPPLE_UTILS_P_H
#define QAPPLE_UTILS_P_H

#include <QtCore/qdebug.h>
#include <QtCore/qendian.h>
#include <QtCore/qglobal.h>

#ifdef Q_OS_MACOS
#  include <CoreAudioTypes/CoreAudioTypes.h>
#endif

QT_BEGIN_NAMESPACE

namespace QtMultimediaPrivate {

struct QOSStatus
{
    OSStatus status;
    explicit QOSStatus(OSStatus s) : status(s) {}

    friend QDebug operator<<(QDebug dbg, const QOSStatus &qos)
    {
        QDebugStateSaver saver(dbg);
        dbg.noquote();

        if (qos.status == noErr) {
            dbg << "noErr";
            return dbg;
        }

        std::array<char, 4> buf;
        qToBigEndian(qos.status, buf.data());

        bool isPrintable = std::all_of(buf.begin(), buf.end(), [](unsigned char c) {
            return std::isprint(c);
        });

        if (isPrintable)
            return dbg << QLatin1String(buf.data(), buf.size());
        else
            return dbg << qos.status;
    }
};

} // namespace QtMultimediaPrivate

QT_END_NAMESPACE

#endif // QAPPLE_UTILS_P_H
