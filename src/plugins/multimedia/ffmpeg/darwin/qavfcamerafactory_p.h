// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QAVFCAMERAFACTORY_P_H
#define QAVFCAMERAFACTORY_P_H

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

#include <QtMultimedia/private/qplatformcamera_p.h>

#include <memory>

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

[[nodiscard]] std::unique_ptr<QPlatformCamera> makeQAvfCamera(QCamera &);

} // namespace QFFmpeg

QT_END_NAMESPACE

#endif // QAVFCAMERAFACTORY_P_H
