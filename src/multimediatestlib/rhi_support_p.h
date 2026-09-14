// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

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

#ifndef RHI_SUPPORT_P_H
#define RHI_SUPPORT_P_H

#include <QtGui/qoffscreensurface.h>
#include <QtGui/rhi/qrhi.h>
#include <QtCore/private/qexpected_p.h>

#include <memory>

namespace QtMultimediaTest {

#if QT_CONFIG(opengl)
std::unique_ptr<QRhi> createOffscreenGlRhi(std::unique_ptr<QOffscreenSurface> &fallbackSurface);
#endif

#if QT_CONFIG(metal)
std::unique_ptr<QRhi> createOffscreenMetalRhi();
#endif

q23::expected<QByteArray, QString> readBackPlane(QRhi &, quint64 handle, QSize planeSize,
                                                 QRhiTexture::Format);

} // namespace QtMultimediaTest

#endif // RHI_SUPPORT_P_H
