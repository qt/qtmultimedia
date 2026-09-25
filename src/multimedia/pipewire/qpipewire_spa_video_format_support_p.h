// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPIPEWIRE_SPA_VIDEO_FORMAT_SUPPORT_P_H
#define QPIPEWIRE_SPA_VIDEO_FORMAT_SUPPORT_P_H

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
#include <QtMultimedia/qcameradevice.h>
#include <QtMultimedia/qvideoframeformat.h>
#include <QtCore/qlist.h>
#include <QtCore/qsize.h>
#include <QtCore/qspan.h>

#if __has_include(<spa/param/format.h>)
#  include <spa/param/format.h>
#else
#  include <QtMultimedia/private/qpipewire_spa_compat_p.h>
#endif
#include <spa/pod/pod.h>

#include <optional>
#include <variant>
#include <vector>

QT_BEGIN_NAMESPACE

namespace QtPipeWire {

struct SizeRange
{
    QSize preferred;
    QSize min;
    QSize max;
};

struct Q_MULTIMEDIA_EXPORT SpaObjectVideoFormat
{
    static std::optional<SpaObjectVideoFormat> parse(const spa_pod *);

    const spa_media_subtype mediaSubtype = SPA_MEDIA_SUBTYPE_raw;

    std::vector<QVideoFrameFormat::PixelFormat> pixelFormats;

    using ResolutionSet = std::variant<std::vector<QSize>, SizeRange>;
    ResolutionSet resolutions;

    qreal minFrameRate = 0.;
    qreal maxFrameRate = 0.;
};

// Cross product of the pixel formats and resolutions of 'formats'
Q_MULTIMEDIA_EXPORT QList<QCameraFormat> toCameraFormats(QSpan<const SpaObjectVideoFormat>);

} // namespace QtPipeWire

QT_END_NAMESPACE

#endif // QPIPEWIRE_SPA_VIDEO_FORMAT_SUPPORT_P_H
