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
#include <QtMultimedia/private/qwindowsd3d11planesplitter_p.h>
#include <QtMultimedia/private/qwindowsd3d11texturebridge_p.h>

#include <QtGui/qtguiglobal.h>

#include <QtCore/qobject.h>
#include <QtCore/qsize.h>
#include <QtCore/private/qcomptr_p.h>
#include <QtCore/private/quniquehandle_p.h>

#include <d3d11.h>
#include <d3d11_1.h>

#include <atomic>
#include <memory>
#include <vector>

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

    /** Returns whether a texture of the given pixel format can be imported into 'rhi'. On
     *  D3D12 and OpenGL, multi-plane formats must be splittable by QWindowsD3D11PlaneSplitter. */
    static bool isPixelFormatSupportedByRhiBackend(QRhi &, QVideoFrameFormat::PixelFormat);

    /** 'pixelFormat' is only consulted on D3D12 and OpenGL, to route multi-plane textures
     *  through QWindowsD3D11PlaneSplitter. Returns null on failure. */
    QVideoFrameTexturesHandlesUPtr
    importTexture(QRhi &, ID3D11Device *srcDevice, ID3D11DeviceContext *srcContext,
                  const ComPtr<ID3D11Texture2D> &texture, const QSize &frameSize,
                  QVideoFrameFormat::PixelFormat pixelFormat, UINT subresourceIndex = 0);

private:
    // Textures that D3D12 imports are unwrapped from. A texture stays unwrapped until the handles
    // of its frame are destroyed, and must neither be unwrapped again nor written to before that,
    // so every frame in flight gets a texture of its own. Released textures are reused.
    class D3D12OutputTexturePool
    {
    public:
        struct Texture
        {
            ComPtr<ID3D11Texture2D> texture;
            // Set while the handles of a frame use the texture.
            std::shared_ptr<std::atomic_bool> inUse;
        };

        Texture acquire(ID3D11Device *device, const D3D11_TEXTURE2D_DESC &desc);

    private:
        std::vector<Texture> m_textures;
    };

    QVideoFrameTexturesHandlesUPtr importTextureD3D11(QRhi &, ID3D11Device *srcDevice,
                                                      ID3D11DeviceContext *srcContext,
                                                      const ComPtr<ID3D11Texture2D> &texture,
                                                      const QSize &frameSize,
                                                      UINT subresourceIndex);
    QVideoFrameTexturesHandlesUPtr importTextureD3D12(QRhi &, ID3D11Device *srcDevice,
                                                      ID3D11DeviceContext *srcContext,
                                                      const ComPtr<ID3D11Texture2D> &texture,
                                                      const QSize &frameSize, UINT subresourceIndex,
                                                      QWindowsD3D11TextureBridge &bridge,
                                                      D3D12OutputTexturePool &outputPool);
    QVideoFrameTexturesHandlesUPtr
    importSplitPlanes(QRhi &rhi, ID3D11Device *srcDevice, ID3D11DeviceContext *srcContext,
                      const ComPtr<ID3D11Texture2D> &texture, const QSize &frameSize,
                      QVideoFrameFormat::PixelFormat pixelFormat, UINT subresourceIndex);

    QWindowsD3D11TextureBridge m_bridge; // D3D11 import, and single-plane D3D12/OpenGL import
    QWindowsD3D11TextureBridge m_bridge2; // second plane of a split multi-plane D3D12 import
    std::unique_ptr<QWindowsD3D11PlaneSplitter> m_planeSplitter; // bound to m_planeSplitterDevice
    ComPtr<ID3D11Device> m_planeSplitterDevice; // device m_planeSplitter was created for

    ComPtr<ID3D11Device> m_d3d11on12Device;
    D3D12OutputTexturePool m_d3d12OutputPool; // used with m_bridge
    D3D12OutputTexturePool m_d3d12OutputPool2; // used with m_bridge2

#if QT_CONFIG(opengl)
    QVideoFrameTexturesHandlesUPtr importTextureOpenGL(QRhi &rhi, ID3D11Device *srcDevice,
                                                       ID3D11DeviceContext *srcContext,
                                                       const ComPtr<ID3D11Texture2D> &texture,
                                                       const QSize &frameSize,
                                                       ComPtr<ID3D11Texture2D> &interopTexSlot);
    void releaseGLInterop();

    // OpenGL interop (WGL_NV_DX_interop2)
    WglDxDeviceHandle m_glInteropDevice;
    ComPtr<ID3D11Texture2D> m_glInteropTex; // cached non-shared texture; single-plane import
    ComPtr<ID3D11Texture2D> m_glInteropTex2; // cached non-shared texture; second plane when split
    std::unique_ptr<QObject> m_glCleanupContext; // context object for the cleanup connection
#endif
};

} // namespace QtMultimediaPrivate

QT_END_NAMESPACE

#endif // QWINDOWSD3D11RHITEXTUREIMPORTER_P_H
