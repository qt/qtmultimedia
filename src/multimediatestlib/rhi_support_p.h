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

#if defined(Q_OS_WIN)
#  include <QtMultimediaTestLib/private/qwindowsd3d11testdevicecontext_p.h>
#endif

namespace QtMultimediaTest {

std::unique_ptr<QRhi> createNullRhi();

#if QT_CONFIG(opengl)
struct OffscreenGlRhi
{
    // the QRhi must be destroyed before the QOffscreenSurface it was built from.
    std::unique_ptr<QOffscreenSurface> fallbackSurface;
    std::unique_ptr<QRhi> rhi;
};

q23::expected<OffscreenGlRhi, QString> createOffscreenGlRhi();
#endif

#if QT_CONFIG(metal)
std::unique_ptr<QRhi> createOffscreenMetalRhi();
#endif

#if defined(Q_OS_WIN)
std::unique_ptr<QRhi> createOffscreenD3D11Rhi();

std::unique_ptr<QRhi> createWarpD3D11Rhi(const QWindowsD3D11TestDeviceContext &warpDevice);
#endif

q23::expected<QByteArray, QString> readBackPlane(QRhi &, quint64 handle, QSize planeSize,
                                                 QRhiTexture::Format);

#if QT_CONFIG(opengl)
q23::expected<QByteArray, QString> readBackExternalOesTexture(QRhi &, quint64 handle, QSize size);
#endif

} // namespace QtMultimediaTest

#endif // RHI_SUPPORT_P_H
