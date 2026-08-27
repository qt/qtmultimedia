// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPIPEWIRE_VIDEOFORMAT_SUPPORT_P_H
#define QPIPEWIRE_VIDEOFORMAT_SUPPORT_P_H

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

#include <QtMultimedia/private/qtmultimediaglobal_p.h>
#include <QtMultimedia/qvideoframeformat.h>

#include <spa/param/video/raw.h>
#include <spa/utils/defs.h>

QT_BEGIN_NAMESPACE

namespace QtPipeWire {

struct FrameRate
{
    qreal fps; // Used as stream frame rate
    spa_fraction frac; // Used as pipewire frame rate
};

Q_MULTIMEDIA_EXPORT FrameRate rateFromFps(qreal fps);

Q_MULTIMEDIA_EXPORT QVideoFrameFormat::PixelFormat toQtPixelFormat(spa_video_format);
Q_MULTIMEDIA_EXPORT spa_video_format toSpaVideoFormat(QVideoFrameFormat::PixelFormat);

inline QSize toQSize(spa_rectangle rect)
{
    return QSize{
        int(rect.width),
        int(rect.height),
    };
}

} // namespace QtPipeWire

QT_END_NAMESPACE

#endif // QPIPEWIRE_VIDEOFORMAT_SUPPORT_P_H
