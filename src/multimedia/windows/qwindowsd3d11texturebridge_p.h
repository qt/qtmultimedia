// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QWINDOWSD3D11TEXTUREBRIDGE_P_H
#define QWINDOWSD3D11TEXTUREBRIDGE_P_H

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

#include <QtCore/qsize.h>
#include <QtCore/private/qcomptr_p.h>
#include <QtCore/private/quniquehandle_types_p.h>

#include <d3d11.h>
#include <d3d11_1.h>

QT_BEGIN_NAMESPACE

namespace QtMultimediaPrivate {

/*! \internal Utility class for synchronized transfer of a texture between two D3D devices
 *
 * This class is used to copy a texture from one device to another device. This
 * is implemented using a shared texture, along with keyed mutexes to synchronize
 * access to the texture.
 *
 * This is needed because the source and destination may use different D3D devices,
 * for example when copying a decoded frame owned by FFmpeg or the Windows Media
 * Foundation media engine into a texture usable by RHI.
 */
class Q_MULTIMEDIA_EXPORT QWindowsD3D11TextureBridge final
{
public:
    /** Copy a texture slice at position 'index' belonging to device 'dev'
     * into a shared texture, limiting the texture size to the frame size */
    bool copyToSharedTex(ID3D11Device *, ID3D11DeviceContext *, const ComPtr<ID3D11Texture2D> &,
                         const QSize &frameSize, UINT index = 0);
    bool copyToSharedTex(const ComPtr<ID3D11Device> &, const ComPtr<ID3D11DeviceContext> &,
                         const ComPtr<ID3D11Texture2D> &, const QSize &frameSize, UINT index = 0);

    /** Obtain a copy of the texture on a second device 'dev'.
     * NOTE: Will block (up to a timeout) until a texture is available
     * (copyToSharedTex was called) */
    ComPtr<ID3D11Texture2D> copyFromSharedTex(const ComPtr<ID3D11Device1> &,
                                              const ComPtr<ID3D11DeviceContext> &);

private:
    bool ensureDestTex(const ComPtr<ID3D11Device1> &dev);
    bool ensureSrcTex(ID3D11Device *dev, const ComPtr<ID3D11Texture2D> &tex,
                      const QSize &frameSize);
    bool isSrcInitialized(const ID3D11Device *dev, const ComPtr<ID3D11Texture2D> &tex,
                          const QSize &frameSize) const;
    bool recreateSrc(ID3D11Device *dev, const ComPtr<ID3D11Texture2D> &tex, const QSize &frameSize);

    QUniqueWin32NullHandle m_sharedHandle{};

    static constexpr UINT kSrcKey = 0;
    static constexpr UINT kDestKey = 1;

    ComPtr<ID3D11Texture2D> m_srcTex;
    ComPtr<IDXGIKeyedMutex> m_srcMutex;

    ComPtr<ID3D11Device1> m_destDevice;
    ComPtr<ID3D11Texture2D> m_destTex;
    ComPtr<IDXGIKeyedMutex> m_destMutex;

    ComPtr<ID3D11Texture2D> m_outputTex;
};

} // namespace QtMultimediaPrivate

QT_END_NAMESPACE

#endif
