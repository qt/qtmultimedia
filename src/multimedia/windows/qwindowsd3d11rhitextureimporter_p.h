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

#include <QtGui/qtguiglobal.h>

#include <QtCore/qobject.h>
#include <QtCore/qsize.h>
#include <QtCore/private/qcomptr_p.h>
#include <QtCore/private/quniquehandle_p.h>

#include <d3d11.h>
#include <d3d11_1.h>

#include <memory>

QT_BEGIN_NAMESPACE

class QRhi;

namespace QtMultimediaPrivate {

#if QT_CONFIG(opengl)
class WglDxDeviceDeleter
{
public:
    WglDxDeviceDeleter() = default;
    explicit WglDxDeviceDeleter(BOOL(WINAPI *closeFn)(HANDLE)) : m_closeFn(closeFn) { }
    void operator()(HANDLE device) const noexcept
    {
        Q_ASSERT(m_closeFn);
        m_closeFn(device);
    }

private:
    BOOL(WINAPI *m_closeFn)(HANDLE) = nullptr;
};

struct WglDxDeviceHandleTraits
{
    using Type = HANDLE;
    static Type invalidValue() noexcept { return nullptr; }
};
using WglDxDeviceHandle = QUniqueHandle<WglDxDeviceHandleTraits, WglDxDeviceDeleter>;
#endif

class Q_MULTIMEDIA_EXPORT QWindowsD3D11RhiTextureImporter
{
public:
    QWindowsD3D11RhiTextureImporter();
    ~QWindowsD3D11RhiTextureImporter();

    Q_DISABLE_COPY_MOVE(QWindowsD3D11RhiTextureImporter)

    static bool isRhiBackendSupported(QRhi &);

    // D3D12 and OpenGL only supports single-plane formats: it has no way to view distinct planes
    // of one texture with different formats, so a multi-planar format (e.g. NV12) must be converted
    // to a single-plane one first
    static bool isPixelFormatSupportedByRhiBackend(QRhi &, QVideoFrameFormat::PixelFormat);

    /** Imports the texture slice at position 'subresourceIndex' of 'texture', owned by
     *  'srcDevice', so that it can be used by 'rhi'. Returns null on failure. */
    QVideoFrameTexturesHandlesUPtr importTexture(QRhi &, ID3D11Device *srcDevice,
                                                 ID3D11DeviceContext *srcContext,
                                                 const ComPtr<ID3D11Texture2D> &texture,
                                                 const QSize &frameSize, UINT subresourceIndex = 0);

private:
    QVideoFrameTexturesHandlesUPtr importTextureD3D11(QRhi &, ID3D11Device *srcDevice,
                                                      ID3D11DeviceContext *srcContext,
                                                      const ComPtr<ID3D11Texture2D> &texture,
                                                      const QSize &frameSize,
                                                      UINT subresourceIndex);
    QVideoFrameTexturesHandlesUPtr importTextureD3D12(QRhi &, ID3D11Device *srcDevice,
                                                      ID3D11DeviceContext *srcContext,
                                                      const ComPtr<ID3D11Texture2D> &texture,
                                                      const QSize &frameSize,
                                                      UINT subresourceIndex);

    QWindowsD3D11TextureBridge m_bridge;

    ComPtr<ID3D11Device> m_d3d11on12Device;

#if QT_CONFIG(opengl)
    QVideoFrameTexturesHandlesUPtr importTextureOpenGL(QRhi &rhi, ID3D11Device *srcDevice,
                                                       ID3D11DeviceContext *srcContext,
                                                       const ComPtr<ID3D11Texture2D> &texture,
                                                       const QSize &frameSize);
    void releaseGLInterop();

    // OpenGL interop (WGL_NV_DX_interop2)
    WglDxDeviceHandle m_glInteropDevice;
    ComPtr<ID3D11Texture2D> m_glInteropTex; // cached non-shared texture for WGL interop
    std::unique_ptr<QObject> m_glCleanupContext; // context object for the cleanup connection
#endif
};

} // namespace QtMultimediaPrivate

QT_END_NAMESPACE

#endif // QWINDOWSD3D11RHITEXTUREIMPORTER_P_H
