// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#define QT_MULTIMEDIA_BUILD_REMOVED_API

#include <QtMultimedia/qtmultimediaglobal.h>

#if QT_MULTIMEDIA_REMOVED_SINCE(6, 10) && !defined(Q_QDOC)

#include "qaudio.h"

QT_BEGIN_NAMESPACE

namespace QAudio {
float convertVolume(float volume, VolumeScale from, VolumeScale to)
{
    return QtAudio::convertVolume(volume, from, to);
}
} // namespace QAudio

QT_END_NAMESPACE

#endif // QT_MULTIMEDIA_REMOVED_SINCE(6, 10) && !defined(Q_QDOC)
