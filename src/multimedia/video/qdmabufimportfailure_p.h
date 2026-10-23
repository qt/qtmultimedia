// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QDMABUFIMPORTFAILURE_P_H
#define QDMABUFIMPORTFAILURE_P_H

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

QT_BEGIN_NAMESPACE

namespace QtMultimediaPrivate {

// Severity of a failure to import DMABUF planes as native (GL/Vulkan) textures.
enum class FailureSeverity {
    recoverable, // the caller may retry, e.g. with a different frame
    unrecoverable, // the caller should permanently disable DMABUF import
};

} // namespace QtMultimediaPrivate

QT_END_NAMESPACE

#endif // QDMABUFIMPORTFAILURE_P_H
