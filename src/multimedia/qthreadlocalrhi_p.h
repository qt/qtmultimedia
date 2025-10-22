// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTHREADLOCALRHI_P_H
#define QTHREADLOCALRHI_P_H

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

#include <QtMultimedia/qtmultimediaexports.h>

#include <QtGui/rhi/qrhi.h>

QT_BEGIN_NAMESPACE

Q_MULTIMEDIA_EXPORT QRhi *qEnsureThreadLocalRhi(QRhi *referenceRhi = nullptr);

// Used only for testing
Q_MULTIMEDIA_EXPORT void qSetPreferredThreadLocalRhiBackend(QRhi::Implementation backend);

QT_END_NAMESPACE

#endif // QTHREADLOCALRHI_P_H
