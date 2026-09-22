// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwindowsd3d11rhitextureimporter_p.h"

#include <QtMultimedia/private/qvideotexturehelper_p.h>

#include <QtGui/rhi/qrhi.h>

#include <QtCore/qloggingcategory.h>
#include <QtCore/qscopeguard.h>
#include <QtCore/qvarlengtharray.h>
#include <QtCore/private/qsystemerror_p.h>
#include <QtCore/private/qexpected_p.h>

#include <d3d11on12.h>
#include <d3d12.h>

#include <algorithm>

#if QT_CONFIG(opengl)
#  include <QtMultimedia/private/qmultimedia_gl_support_p.h>
#  include <QtMultimedia/private/qrhivaluemapper_p.h>
#  include <QtGui/qopenglcontext.h>
#  include <QtGui/qopenglextrafunctions.h>
#  include <QtGui/qopenglfunctions.h>
#  include <QtGui/qopenglext.h>
#endif

QT_BEGIN_NAMESPACE

namespace QtMultimediaPrivate {

using namespace Qt::Literals;

namespace {

Q_LOGGING_CATEGORY(lcD3D11RhiTextureImporter, "qt.multimedia.windows.d3d11rhitextureimporter");

class D3D11TextureHandles : public QVideoFrameTexturesHandles
{
public:
    explicit D3D11TextureHandles(ComPtr<ID3D11Texture2D> tex) : m_tex(std::move(tex)) { }

    quint64 textureHandle(QRhi &rhi, int /*plane*/) override
    {
        if (rhi.backend() != QRhi::D3D11)
            return 0;
        return quint64(m_tex.Get());
    }

private:
    const ComPtr<ID3D11Texture2D> m_tex;
};

const QRhiD3D11NativeHandles *resolveD3D11NativeHandles(QRhi &rhi)
{
    Q_ASSERT(rhi.backend() == QRhi::D3D11);
    return static_cast<const QRhiD3D11NativeHandles *>(rhi.nativeHandles());
}

q23::expected<ComPtr<ID3D11Device1>, HRESULT>
resolveD3D11Device(const QRhiD3D11NativeHandles *nativeHandles)
{
    ComPtr<ID3D11Device> rhiDevice = static_cast<ID3D11Device *>(nativeHandles->dev);
    ComPtr<ID3D11Device1> rhiDevice1;
    if (const HRESULT hr = rhiDevice.As(&rhiDevice1); FAILED(hr))
        return q23::unexpected{ hr };
    return rhiDevice1;
}

// Exposes the D3D12 view of a D3D11 texture, unwrapped via D3D11On12
class D3D12TextureHandles : public QVideoFrameTexturesHandles
{
public:
    D3D12TextureHandles(ComPtr<ID3D12Resource> resource, ComPtr<ID3D11On12Device2> on12Device,
                        ComPtr<ID3D11Texture2D> wrappedResource,
                        std::shared_ptr<std::atomic_bool> wrappedResourceInUse)
        : m_resource(std::move(resource)),
          m_on12Device(std::move(on12Device)),
          m_wrappedResource(std::move(wrappedResource)),
          m_wrappedResourceInUse(std::move(wrappedResourceInUse))
    {
    }

    ~D3D12TextureHandles() override
    {
        m_resource.Reset();
        if (m_on12Device && m_wrappedResource)
            m_on12Device->ReturnUnderlyingResource(m_wrappedResource.Get(), 0, nullptr, nullptr);
        // Only now may the texture be reused for another frame.
        if (m_wrappedResourceInUse)
            m_wrappedResourceInUse->store(false);
    }

    quint64 textureHandle(QRhi &rhi, int /*plane*/) override
    {
        switch (rhi.backend()) {
        case QRhi::D3D12:
            return quint64(m_resource.Get());
        default:
            return 0;
        }
    }

private:
    ComPtr<ID3D12Resource> m_resource;
    const ComPtr<ID3D11On12Device2> m_on12Device;
    const ComPtr<ID3D11Texture2D> m_wrappedResource;
    const std::shared_ptr<std::atomic_bool> m_wrappedResourceInUse;
};

#if QT_CONFIG(opengl)

// WGL_NV_DX_interop2 function types
using PFNWGLDXOPENDEVICENVPROC = HANDLE(WINAPI *)(void *);
using PFNWGLDXCLOSEDEVICENVPROC = BOOL(WINAPI *)(HANDLE);
using PFNWGLDXREGISTEROBJECTNVPROC = HANDLE(WINAPI *)(HANDLE, void *, GLuint, GLenum, GLenum);
using PFNWGLDXUNREGISTEROBJECTNVPROC = BOOL(WINAPI *)(HANDLE, HANDLE);
using PFNWGLDXLOCKOBJECTSNVPROC = BOOL(WINAPI *)(HANDLE, GLint, HANDLE *);
using PFNWGLDXUNLOCKOBJECTSNVPROC = BOOL(WINAPI *)(HANDLE, GLint, HANDLE *);

#  ifndef WGL_ACCESS_READ_ONLY_NV
#    define WGL_ACCESS_READ_ONLY_NV 0x00000000
#  endif

struct WglDxInterop
{
    PFNWGLDXOPENDEVICENVPROC wglDXOpenDeviceNV = nullptr;
    PFNWGLDXCLOSEDEVICENVPROC wglDXCloseDeviceNV = nullptr;
    PFNWGLDXREGISTEROBJECTNVPROC wglDXRegisterObjectNV = nullptr;
    PFNWGLDXUNREGISTEROBJECTNVPROC wglDXUnregisterObjectNV = nullptr;
    PFNWGLDXLOCKOBJECTSNVPROC wglDXLockObjectsNV = nullptr;
    PFNWGLDXUNLOCKOBJECTSNVPROC wglDXUnlockObjectsNV = nullptr;

    static WglDxInterop resolve(QOpenGLContext *ctx)
    {
        using namespace QtMultimediaPrivate;
        WglDxInterop f;
        if (!ctx)
            return f;
        f.wglDXOpenDeviceNV = resolveGlFunction<PFNWGLDXOPENDEVICENVPROC>(ctx, "wglDXOpenDeviceNV");
        f.wglDXCloseDeviceNV =
                resolveGlFunction<PFNWGLDXCLOSEDEVICENVPROC>(ctx, "wglDXCloseDeviceNV");
        f.wglDXRegisterObjectNV =
                resolveGlFunction<PFNWGLDXREGISTEROBJECTNVPROC>(ctx, "wglDXRegisterObjectNV");
        f.wglDXUnregisterObjectNV =
                resolveGlFunction<PFNWGLDXUNREGISTEROBJECTNVPROC>(ctx, "wglDXUnregisterObjectNV");
        f.wglDXLockObjectsNV =
                resolveGlFunction<PFNWGLDXLOCKOBJECTSNVPROC>(ctx, "wglDXLockObjectsNV");
        f.wglDXUnlockObjectsNV =
                resolveGlFunction<PFNWGLDXUNLOCKOBJECTSNVPROC>(ctx, "wglDXUnlockObjectsNV");
        return f;
    }

    bool valid() const
    {
        return wglDXOpenDeviceNV && wglDXCloseDeviceNV && wglDXRegisterObjectNV
                && wglDXUnregisterObjectNV && wglDXLockObjectsNV && wglDXUnlockObjectsNV;
    }
};

QRhiValueMapper<WglDxInterop> g_wglDxInteropMapper;

bool supportsGlCopy(QOpenGLContext &glCtx)
{
    return (glCtx.format().version() >= std::pair(4, 3)
            || glCtx.hasExtension("GL_ARB_copy_image"_ba));
}

GLenum dxFormatToGlTextureFormat(DXGI_FORMAT d3dFormat)
{
    switch (d3dFormat) {
    case DXGI_FORMAT_R8G8B8A8_UNORM:
        return GL_RGBA8;
    case DXGI_FORMAT_R8_UNORM:
        return GL_R8;
    case DXGI_FORMAT_R8G8_UNORM:
        return GL_RG8;
    case DXGI_FORMAT_R16_UNORM:
        return GL_R16;
    case DXGI_FORMAT_R16G16_UNORM:
        return GL_RG16;
    default:
        return GLenum(0);
    }
}

// Unlike the D3D11/D3D12 handles, which keep their shared source texture alive, the GL result
// must survive the transient WGL registration (see importTextureOpenGL): makes a GPU-side copy
// of 'source' into a standalone texture with its own storage. Leaves the GL_TEXTURE_2D binding
// as found, since QRhi tracks GL state.
GlTextureHandle copyToOwnedGlTexture(QOpenGLContext *glCtx, GLuint source, DXGI_FORMAT sourceFormat,
                                     GLsizei width, GLsizei height)
{
    Q_ASSERT(supportsGlCopy(*glCtx));

    const GLenum internalFormat = dxFormatToGlTextureFormat(sourceFormat);
    if (!internalFormat) {
        qCWarning(lcD3D11RhiTextureImporter)
                << "Cannot import D3D11 texture to OpenGL: unsupported DXGI format" << sourceFormat;
        return {};
    }

    QOpenGLExtraFunctions *f = glCtx->extraFunctions();
    clearGlErrors(f);
    const auto checkGlError = [&](const char *call) {
        const GLError error = GLError(f->glGetError());
        if (error == GLError::NoError)
            return true;
        qCWarning(lcD3D11RhiTextureImporter) << call << "failed with GL error" << error;
        return false;
    };

    GLint previousBinding = 0;
    f->glGetIntegerv(GL_TEXTURE_BINDING_2D, &previousBinding);
    if (!checkGlError("glGetIntegerv(GL_TEXTURE_BINDING_2D)"))
        return {};
    auto restoreBinding = qScopeGuard([&] {
        f->glBindTexture(GL_TEXTURE_2D, GLuint(previousBinding));
    });

    GLuint texId = 0;
    f->glGenTextures(1, &texId);
    if (!checkGlError("glGenTextures") || !texId)
        return {};
    GlTextureHandle result{
        texId,
        GlTextureDeleter{ glCtx },
    };

    f->glBindTexture(GL_TEXTURE_2D, texId);
    if (!checkGlError("glBindTexture"))
        return {};

    f->glTexStorage2D(GL_TEXTURE_2D, 1, internalFormat, width, height);
    if (!checkGlError("glTexStorage2D"))
        return {};

    f->glCopyImageSubData(source, GL_TEXTURE_2D, 0, 0, 0, 0, texId, GL_TEXTURE_2D, 0, 0, 0, 0,
                          width, height, 1);
    if (!checkGlError("glCopyImageSubData"))
        return {};

    return result;
}

class OpenGLTextureHandles : public QVideoFrameTexturesHandles
{
public:
    explicit OpenGLTextureHandles(GlTextureHandle tex) : m_tex(std::move(tex)) { }

    quint64 textureHandle(QRhi &rhi, int /*plane*/) override
    {
        if (rhi.backend() != QRhi::OpenGLES2)
            return 0;
        return quint64(m_tex.get());
    }

private:
    const GlTextureHandle m_tex;
};

#endif // QT_CONFIG(opengl)

// At most 2 planes: NV12 and P010 are luma plus a single chroma plane.
class MultiPlaneTextureHandles : public QVideoFrameTexturesHandles
{
public:
    explicit MultiPlaneTextureHandles(QVarLengthArray<QVideoFrameTexturesHandlesUPtr, 2> planes)
        : m_planes(std::move(planes))
    {
    }

    quint64 textureHandle(QRhi &rhi, int plane) override
    {
        if (plane < 0 || plane >= m_planes.size() || !m_planes[plane])
            return 0;
        return m_planes[plane]->textureHandle(rhi, 0);
    }

private:
    const QVarLengthArray<QVideoFrameTexturesHandlesUPtr, 2> m_planes;
};

} // namespace

QWindowsD3D11RhiTextureImporter::QWindowsD3D11RhiTextureImporter() = default;
QWindowsD3D11RhiTextureImporter::~QWindowsD3D11RhiTextureImporter() = default;

QWindowsD3D11RhiTextureImporter::D3D12OutputTexturePool::Texture
QWindowsD3D11RhiTextureImporter::D3D12OutputTexturePool::acquire(ID3D11Device *device,
                                                                 const D3D11_TEXTURE2D_DESC &desc)
{
    const auto matches = [&](const Texture &entry) {
        ComPtr<ID3D11Device> entryDevice;
        entry.texture->GetDevice(&entryDevice);
        D3D11_TEXTURE2D_DESC entryDesc{};
        entry.texture->GetDesc(&entryDesc);
        return entryDevice.Get() == device && entryDesc.Format == desc.Format
                && entryDesc.Width == desc.Width && entryDesc.Height == desc.Height;
    };

    m_textures.erase(std::remove_if(m_textures.begin(), m_textures.end(),
                                    [&](const Texture &entry) {
        return !entry.inUse->load() && !matches(entry);
    }),
                     m_textures.end());

    for (const Texture &entry : m_textures) {
        if (!entry.inUse->load() && matches(entry)) {
            entry.inUse->store(true);
            return entry;
        }
    }

    Texture entry{ {}, std::make_shared<std::atomic_bool>(true) };
    if (const HRESULT hr = device->CreateTexture2D(&desc, nullptr, &entry.texture); FAILED(hr)) {
        qCWarning(lcD3D11RhiTextureImporter)
                << "CreateTexture2D failed for D3D12 import:" << QSystemError::windowsComString(hr);
        return {};
    }
    m_textures.push_back(entry);
    return entry;
}

bool QWindowsD3D11RhiTextureImporter::isRhiBackendSupported(QRhi &rhi)
{
    switch (rhi.backend()) {
    case QRhi::D3D11: {
        // A software (WARP) D3D11 device lives on a different adapter than the source device,
        // so opening a shared handle across them would either fail or, worse, block on the
        // keyed mutex until the finite timeout expires. Treat it the same as unsupported.
        if (rhi.driverInfo().deviceType == QRhiDriverInfo::CpuDevice)
            return false;

        const QRhiD3D11NativeHandles *nh = resolveD3D11NativeHandles(rhi);
        return nh && nh->dev;
    }
    case QRhi::D3D12:
        return true;
#if QT_CONFIG(opengl)
    case QRhi::OpenGLES2: {
        const auto *nh = static_cast<const QRhiGles2NativeHandles *>(rhi.nativeHandles());
        if (!nh || !nh->context)
            return false;
        if (!supportsGlCopy(*nh->context))
            return false; // GL_ARB_copy_image or OpenGL >= 4.3 is required to copy the D3D texture into a GL texture

        QOpenGLContext *glCtx = nh->context;
        WglDxInterop &wgl = g_wglDxInteropMapper.getOrCreate(rhi, [&] {
            return WglDxInterop::resolve(glCtx);
        });
        return wgl.valid();
    }
#endif
    default:
        return false;
    }
}

bool QWindowsD3D11RhiTextureImporter::isPixelFormatSupportedByRhiBackend(
        QRhi &rhi, QVideoFrameFormat::PixelFormat pixelFormat)
{
    switch (rhi.backend()) {
    case QRhi::D3D12:
    case QRhi::OpenGLES2: {
        const auto *desc = QVideoTextureHelper::textureDescription(pixelFormat);
        if (desc && desc->nplanes == 1)
            return true;
        return QWindowsD3D11PlaneSplitter::isPixelFormatSupported(pixelFormat);
    }
    default:
        return true;
    }
}

QVideoFrameTexturesHandlesUPtr QWindowsD3D11RhiTextureImporter::importTexture(
        QRhi &rhi, ID3D11Device *srcDevice, ID3D11DeviceContext *srcContext,
        const ComPtr<ID3D11Texture2D> &texture, const QSize &frameSize,
        QVideoFrameFormat::PixelFormat pixelFormat, UINT subresourceIndex)
{
    if (rhi.backend() == QRhi::D3D12 || rhi.backend() == QRhi::OpenGLES2) {
        const auto *desc = QVideoTextureHelper::textureDescription(pixelFormat);
        if (desc && desc->nplanes > 1)
            return importSplitPlanes(rhi, srcDevice, srcContext, texture, frameSize, pixelFormat,
                                     subresourceIndex);
    }

    switch (rhi.backend()) {
    case QRhi::D3D11:
        return importTextureD3D11(rhi, srcDevice, srcContext, texture, frameSize, subresourceIndex);
    case QRhi::D3D12:
        return importTextureD3D12(rhi, srcDevice, srcContext, texture, frameSize, subresourceIndex,
                                  m_bridge, m_d3d12OutputPool);
#if QT_CONFIG(opengl)
    case QRhi::OpenGLES2:
        return importTextureOpenGL(rhi, srcDevice, srcContext, texture, frameSize, m_glInteropTex);
#endif
    default:
        return nullptr;
    }
}

QVideoFrameTexturesHandlesUPtr QWindowsD3D11RhiTextureImporter::importSplitPlanes(
        QRhi &rhi, ID3D11Device *srcDevice, ID3D11DeviceContext *srcContext,
        const ComPtr<ID3D11Texture2D> &texture, const QSize &frameSize,
        QVideoFrameFormat::PixelFormat pixelFormat, UINT subresourceIndex)
{
    Q_ASSERT(rhi.backend() == QRhi::D3D12 || rhi.backend() == QRhi::OpenGLES2);

    if (!m_planeSplitter || m_planeSplitterDevice.Get() != srcDevice) {
        m_planeSplitter = QWindowsD3D11PlaneSplitter::create(srcDevice);
        m_planeSplitterDevice = srcDevice;
        if (!m_planeSplitter) {
            qCWarning(lcD3D11RhiTextureImporter) << "Cannot create plane splitter";
            return nullptr;
        }
    }

    const std::optional<std::vector<QWindowsD3D11PlaneSplitter::Plane>> planes =
            m_planeSplitter->split(srcDevice, srcContext, texture, subresourceIndex, frameSize,
                                   pixelFormat);
    if (!planes)
        return nullptr;

    if (planes->size() > 2) {
        qCWarning(lcD3D11RhiTextureImporter) << "Splitting more than 2 planes is not supported";
        return nullptr;
    }

    QWindowsD3D11TextureBridge *bridges[] = { &m_bridge, &m_bridge2 };
    D3D12OutputTexturePool *outputPools[] = { &m_d3d12OutputPool, &m_d3d12OutputPool2 };
#if QT_CONFIG(opengl)
    ComPtr<ID3D11Texture2D> *glInteropSlots[] = { &m_glInteropTex, &m_glInteropTex2 };
#endif

    QVarLengthArray<QVideoFrameTexturesHandlesUPtr, 2> planeHandles;
    for (size_t i = 0; i < planes->size(); ++i) {
        QVideoFrameTexturesHandlesUPtr handles;
        if (rhi.backend() == QRhi::D3D12) {
            handles = importTextureD3D12(rhi, srcDevice, srcContext, (*planes)[i].texture,
                                         (*planes)[i].size, 0, *bridges[i], *outputPools[i]);
        }
#if QT_CONFIG(opengl)
        else {
            handles = importTextureOpenGL(rhi, srcDevice, srcContext, (*planes)[i].texture,
                                          (*planes)[i].size, *glInteropSlots[i]);
        }
#endif
        if (!handles)
            return nullptr;
        planeHandles.push_back(std::move(handles));
    }

    return std::make_unique<MultiPlaneTextureHandles>(std::move(planeHandles));
}

QVideoFrameTexturesHandlesUPtr QWindowsD3D11RhiTextureImporter::importTextureD3D11(
        QRhi &rhi, ID3D11Device *srcDevice, ID3D11DeviceContext *srcContext,
        const ComPtr<ID3D11Texture2D> &texture, const QSize &frameSize, UINT subresourceIndex)
{
    Q_ASSERT(rhi.backend() == QRhi::D3D11);
    const QRhiD3D11NativeHandles *nh = resolveD3D11NativeHandles(rhi);
    Q_ASSERT(nh && nh->dev);

    Q_ASSERT(srcDevice);
    Q_ASSERT(srcContext);
    Q_ASSERT(texture);

    auto device = resolveD3D11Device(nh);
    if (!device) {
        qCWarning(lcD3D11RhiTextureImporter) << "QueryInterface(ID3D11Device1) failed:"
                                             << QSystemError::windowsComString(device.error());
        return nullptr;
    }
    ComPtr<ID3D11Device1> rhiDevice = std::move(*device);

    ComPtr<ID3D11DeviceContext> rhiCtx;
    rhiDevice->GetImmediateContext(&rhiCtx);

    if (!m_bridge.copyToSharedTex(srcDevice, srcContext, texture, frameSize, subresourceIndex))
        return nullptr;

    ComPtr<ID3D11Texture2D> rhiTex = m_bridge.copyFromSharedTex(rhiDevice, rhiCtx);
    if (!rhiTex)
        return nullptr;

    return std::make_unique<D3D11TextureHandles>(std::move(rhiTex));
}

QVideoFrameTexturesHandlesUPtr QWindowsD3D11RhiTextureImporter::importTextureD3D12(
        QRhi &rhi, ID3D11Device *srcDevice, ID3D11DeviceContext *srcContext,
        const ComPtr<ID3D11Texture2D> &texture, const QSize &frameSize, UINT subresourceIndex,
        QWindowsD3D11TextureBridge &bridge, D3D12OutputTexturePool &outputPool)
{
    Q_ASSERT(rhi.backend() == QRhi::D3D12);
    Q_ASSERT(srcDevice);
    Q_ASSERT(srcContext);
    Q_ASSERT(texture);

    const auto *nh = static_cast<const QRhiD3D12NativeHandles *>(rhi.nativeHandles());
    if (!nh || !nh->dev || !nh->commandQueue)
        return nullptr;

    auto *d3d12Device = static_cast<ID3D12Device *>(nh->dev);
    auto *cmdQueue = static_cast<ID3D12CommandQueue *>(nh->commandQueue);

    if (!m_d3d11on12Device) {
        // D3D11On12 needs a queue of its own: UnwrapUnderlyingResource() makes cmdQueue wait
        // for a fence that D3D11On12 only signals in its next submission. If that submission
        // goes to cmdQueue as well, it is queued behind the wait and cmdQueue deadlocks.
        D3D12_COMMAND_QUEUE_DESC queueDesc{};
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        ComPtr<ID3D12CommandQueue> on12Queue;
        if (const HRESULT hr = d3d12Device->CreateCommandQueue(&queueDesc,
                                                               IID_PPV_ARGS(&on12Queue));
            FAILED(hr)) {
            qCWarning(lcD3D11RhiTextureImporter)
                    << "CreateCommandQueue:" << QSystemError::windowsComString(hr);
            return nullptr;
        }

        IUnknown *queues[] = { on12Queue.Get() };
        HRESULT hr = D3D11On12CreateDevice(d3d12Device, 0, nullptr, 0, queues, 1, 0,
                                           &m_d3d11on12Device, nullptr, nullptr);
        if (FAILED(hr)) {
            qCWarning(lcD3D11RhiTextureImporter)
                    << "D3D11On12CreateDevice:" << QSystemError::windowsComString(hr);
            return nullptr;
        }
    }

    ComPtr<ID3D11Device1> d3d11on12Device1;
    if (const HRESULT hr = m_d3d11on12Device.As(&d3d11on12Device1); FAILED(hr)) {
        qCWarning(lcD3D11RhiTextureImporter)
                << "QueryInterface(ID3D11Device1) failed:" << QSystemError::windowsComString(hr);
        return nullptr;
    }

    ComPtr<ID3D11DeviceContext> d3d11on12Ctx;
    m_d3d11on12Device->GetImmediateContext(&d3d11on12Ctx);

    if (!bridge.copyToSharedTex(srcDevice, srcContext, texture, frameSize, subresourceIndex))
        return nullptr;

    D3D11_TEXTURE2D_DESC srcDesc{};
    texture->GetDesc(&srcDesc);
    D3D11_TEXTURE2D_DESC outputDesc{};
    outputDesc.Width = static_cast<UINT>(frameSize.width());
    outputDesc.Height = static_cast<UINT>(frameSize.height());
    outputDesc.MipLevels = 1;
    outputDesc.ArraySize = 1;
    outputDesc.Format = srcDesc.Format;
    outputDesc.SampleDesc.Count = 1;
    outputDesc.Usage = D3D11_USAGE_DEFAULT;
    outputDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D12OutputTexturePool::Texture output = outputPool.acquire(d3d11on12Device1.Get(), outputDesc);
    if (!output.texture)
        return nullptr;
    // Give the texture back to the pool unless handles end up owning it.
    auto releaseOutput = qScopeGuard([&] {
        output.inUse->store(false);
    });

    if (!bridge.copyFromSharedTex(d3d11on12Device1, d3d11on12Ctx, output.texture.Get()))
        return nullptr;

    ComPtr<ID3D11On12Device2> on12Device2;
    if (const HRESULT hr = m_d3d11on12Device.As(&on12Device2); FAILED(hr)) {
        qCWarning(lcD3D11RhiTextureImporter) << "QueryInterface(ID3D11On12Device2) failed:"
                                             << QSystemError::windowsComString(hr);
        return nullptr;
    }

    ComPtr<ID3D12Resource> resource;
    if (const HRESULT hr = on12Device2->UnwrapUnderlyingResource(output.texture.Get(), cmdQueue,
                                                                 IID_PPV_ARGS(&resource));
        FAILED(hr)) {
        qCWarning(lcD3D11RhiTextureImporter)
                << "UnwrapUnderlyingResource:" << QSystemError::windowsComString(hr);
        return nullptr;
    }

    // UnwrapUnderlyingResource() doesn't flush: submit the copy and the fence signal that
    // cmdQueue now waits for.
    d3d11on12Ctx->Flush();

    releaseOutput.dismiss();
    return std::make_unique<D3D12TextureHandles>(std::move(resource), std::move(on12Device2),
                                                 std::move(output.texture),
                                                 std::move(output.inUse));
}

#if QT_CONFIG(opengl)

QVideoFrameTexturesHandlesUPtr QWindowsD3D11RhiTextureImporter::importTextureOpenGL(
        QRhi &rhi, ID3D11Device *srcDevice, ID3D11DeviceContext *srcContext,
        const ComPtr<ID3D11Texture2D> &texture, const QSize &frameSize,
        ComPtr<ID3D11Texture2D> &interopTexSlot)
{
    Q_ASSERT(rhi.backend() == QRhi::OpenGLES2);
    Q_ASSERT(srcDevice);
    Q_ASSERT(srcContext);
    Q_ASSERT(texture);

    const auto *nh = static_cast<const QRhiGles2NativeHandles *>(rhi.nativeHandles());
    if (!nh || !nh->context)
        return nullptr;
    QOpenGLContext *glCtx = nh->context;

    WglDxInterop &wgl = g_wglDxInteropMapper.getOrCreate(rhi, [&] {
        return WglDxInterop::resolve(glCtx);
    });
    if (!wgl.valid()) {
        qCWarning(lcD3D11RhiTextureImporter) << "WGL_NV_DX_interop2 functions not available";
        return nullptr;
    }

    // Reuse or create a non-shared texture matching the frame size for WGL interop (keyed-mutex
    // textures are incompatible with wglDXRegisterObjectNV, and the source texture may be
    // larger than the frame, e.g. due to a decoder's surface alignment requirements).
    const UINT width = static_cast<UINT>(frameSize.width());
    const UINT height = static_cast<UINT>(frameSize.height());
    {
        bool needRecreate = !interopTexSlot;
        if (interopTexSlot) {
            CD3D11_TEXTURE2D_DESC cachedDesc;
            interopTexSlot->GetDesc(&cachedDesc);
            if (cachedDesc.Width != width || cachedDesc.Height != height)
                needRecreate = true;
        }
        if (needRecreate) {
            CD3D11_TEXTURE2D_DESC desc;
            texture->GetDesc(&desc);
            desc.Width = width;
            desc.Height = height;
            desc.ArraySize = 1;
            desc.MiscFlags = 0;
            desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
            if (FAILED(srcDevice->CreateTexture2D(&desc, nullptr,
                                                  interopTexSlot.ReleaseAndGetAddressOf())))
                return nullptr;
        }

        const D3D11_BOX crop{ 0, 0, 0, width, height, 1 };
        srcContext->CopySubresourceRegion(interopTexSlot.Get(), 0, 0, 0, 0, texture.Get(), 0,
                                          &crop);
        srcContext->Flush();
    }

    if (!m_glInteropDevice) {
        HANDLE device = wgl.wglDXOpenDeviceNV(srcDevice);
        if (!device) {
            qCWarning(lcD3D11RhiTextureImporter) << "wglDXOpenDeviceNV failed";
            return nullptr;
        }
        m_glInteropDevice = WglDxDeviceHandle{
            device,
            WglDxDeviceDeleter{ wgl.wglDXCloseDeviceNV },
        };

        // Release the WGL interop device before the GL context is destroyed to avoid leaking
        // the handle.
        if (!m_glCleanupContext)
            m_glCleanupContext = std::make_unique<QObject>();

        QObject::connect(nh->context, &QOpenGLContext::aboutToBeDestroyed, m_glCleanupContext.get(),
                         [this] {
            releaseGLInterop();
        });
    }

    GLuint texId = 0;
    QOpenGLFunctions *f = glCtx->functions();
    f->glGenTextures(1, &texId);
    if (!texId)
        return nullptr;
    GlTextureHandle interopGlTex{
        texId,
        GlTextureDeleter{ glCtx },
    };

    HANDLE interopObj =
            wgl.wglDXRegisterObjectNV(m_glInteropDevice.get(), interopTexSlot.Get(),
                                      interopGlTex.get(), GL_TEXTURE_2D, WGL_ACCESS_READ_ONLY_NV);
    if (!interopObj) {
        qCWarning(lcD3D11RhiTextureImporter) << "wglDXRegisterObjectNV failed";
        return nullptr;
    }
    auto unregisterInteropObj = qScopeGuard([&] {
        wgl.wglDXUnregisterObjectNV(m_glInteropDevice.get(), interopObj);
    });

    if (!wgl.wglDXLockObjectsNV(m_glInteropDevice.get(), 1, &interopObj)) {
        qCWarning(lcD3D11RhiTextureImporter) << "wglDXLockObjectsNV failed";
        return nullptr;
    }

    // GL may only access a registered object while it is locked, and unregistering it detaches
    // 'interopGlTex' from the D3D11 texture. So copy it into a texture that GL owns while it is
    // locked. The copy stays valid after unregistering, and isn't affected when 'interopTexSlot'
    // is reused for the next frame. Locking also synchronizes the D3D11 copy above with GL.
    CD3D11_TEXTURE2D_DESC interopDesc;
    interopTexSlot->GetDesc(&interopDesc);
    GlTextureHandle ownedGlTex = copyToOwnedGlTexture(glCtx, interopGlTex.get(), interopDesc.Format,
                                                      GLsizei(width), GLsizei(height));
    wgl.wglDXUnlockObjectsNV(m_glInteropDevice.get(), 1, &interopObj);

    if (!ownedGlTex)
        return nullptr;

    return std::make_unique<OpenGLTextureHandles>(std::move(ownedGlTex));
}

void QWindowsD3D11RhiTextureImporter::releaseGLInterop()
{
    m_glInteropDevice.reset();
    m_glInteropTex.Reset();
    m_glInteropTex2.Reset();
}

#endif // QT_CONFIG(opengl)

} // namespace QtMultimediaPrivate

QT_END_NAMESPACE
