// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QMULTIMEDIA_VA_SUPPORT_P_H
#define QMULTIMEDIA_VA_SUPPORT_P_H

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

#include <QtMultimedia/private/qtmultimediaglobal_p.h>

#include <va/va.h>

static_assert(QT_CONFIG(vaapi));

QT_BEGIN_NAMESPACE

class QDebug;

namespace QtMultimediaPrivate {

enum class VAStatus : int { Success = VA_STATUS_SUCCESS };

Q_MULTIMEDIA_EXPORT QDebug operator<<(QDebug debug, VAStatus status);

} // namespace QtMultimediaPrivate

QT_END_NAMESPACE

#endif // QMULTIMEDIA_VA_SUPPORT_P_H
