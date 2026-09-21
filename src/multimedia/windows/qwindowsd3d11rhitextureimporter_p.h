// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QWINDOWSD3D11RHITEXTUREIMPORTER_P_H
#define QWINDOWSD3D11RHITEXTUREIMPORTER_P_H

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

#include <QtMultimedia/qtmultimediaexports.h>
#include <QtMultimedia/qvideoframeformat.h>
#include <QtMultimedia/private/qhwvideobuffer_p.h>
#include <QtMultimedia/private/qwindowsd3d11texturebridge_p.h>

#include <QtCore/qsize.h>
#include <QtCore/private/qcomptr_p.h>

#include <d3d11.h>
#include <d3d11_1.h>

QT_BEGIN_NAMESPACE

class QRhi;

namespace QtMultimediaPrivate {

/*! \internal
    Imports a D3D11 texture, owned by an arbitrary D3D11 device, for use by a QRhi.

    Only the QRhi::D3D11 backend is currently supported; this involves a device-to-device copy
    into a shared texture. The instance caches the resources needed to do so cheaply across
    repeated calls (e.g. the shared texture used for the copy), so it should be kept alive for as
    long as textures may need to be imported for a given source device, rather than recreated per
    frame.
*/
class Q_MULTIMEDIA_EXPORT QWindowsD3D11RhiTextureImporter
{
public:
    QWindowsD3D11RhiTextureImporter();
    ~QWindowsD3D11RhiTextureImporter();

    Q_DISABLE_COPY_MOVE(QWindowsD3D11RhiTextureImporter)

    static bool isRhiBackendSupported(QRhi &);

    /** Imports the texture slice at position 'subresourceIndex' of 'texture', owned by
     *  'srcDevice', so that it can be used by 'rhi'. Returns null on failure.
     */
    QVideoFrameTexturesHandlesUPtr importTexture(QRhi &rhi, ID3D11Device *srcDevice,
                                                 ID3D11DeviceContext *srcContext,
                                                 const ComPtr<ID3D11Texture2D> &texture,
                                                 const QSize &frameSize, UINT subresourceIndex = 0);

private:
    QVideoFrameTexturesHandlesUPtr importTextureD3D11(QRhi &rhi, ID3D11Device *srcDevice,
                                                      ID3D11DeviceContext *srcContext,
                                                      const ComPtr<ID3D11Texture2D> &texture,
                                                      const QSize &frameSize,
                                                      UINT subresourceIndex);

    QWindowsD3D11TextureBridge m_bridge;
};

} // namespace QtMultimediaPrivate

QT_END_NAMESPACE

#endif // QWINDOWSD3D11RHITEXTUREIMPORTER_P_H
