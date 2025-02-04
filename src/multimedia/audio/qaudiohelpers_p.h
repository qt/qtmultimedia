// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QAUDIOHELPERS_P_H
#define QAUDIOHELPERS_P_H

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

#include <QtCore/qspan.h>
#include <QtMultimedia/qaudioformat.h>
#include <QtMultimedia/private/qtmultimediaglobal_p.h>
#include <QtMultimedia/private/qaudio_rtsan_support_p.h>

QT_BEGIN_NAMESPACE

namespace QAudioHelperInternal {
Q_MULTIMEDIA_EXPORT void qMultiplySamples(float factor,
                                          const QAudioFormat &format,
                                          const void *src,
                                          void *dest,
                                          int len) QT_MM_NONBLOCKING;

Q_MULTIMEDIA_EXPORT
void applyVolume(float volume,
                 const QAudioFormat &,
                 QSpan<const std::byte> source,
                 QSpan<std::byte> destination) QT_MM_NONBLOCKING;

} // namespace QAudioHelperInternal

QT_END_NAMESPACE

#endif // QAUDIOHELPERS_P_H
