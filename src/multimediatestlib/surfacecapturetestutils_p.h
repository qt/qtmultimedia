// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef SURFACECAPTURETESTUTILS_P_H
#define SURFACECAPTURETESTUTILS_P_H

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

#include <QtCore/qtconfigmacros.h>

#include <memory>

QT_BEGIN_NAMESPACE

class QScreenCapture;
class QWindowCapture;

namespace QtMultimediaTestLib {

[[nodiscard]] std::unique_ptr<QScreenCapture> makeScreenCapture();
[[nodiscard]] std::unique_ptr<QWindowCapture> makeWindowCapture();

} // namespace QtMultimediaTestLib

QT_END_NAMESPACE

#endif // SURFACECAPTURETESTUTILS_P_H
