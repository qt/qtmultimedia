// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "evrd3dpresentengine_p.h"

#include "evrhelpers_p.h"

#include <private/qhwvideobuffer_p.h>
#include <private/qvideoframe_p.h>
#include <qvideoframe.h>
#include <QDebug>
#include <qthread.h>
#include <qvideosink.h>
#include <qloggingcategory.h>

#include <d3d11_1.h>

#include <rhi/qrhi.h>

#if QT_CONFIG(opengl)
#  include <qopenglcontext.h>
#  include <qopenglfunctions.h>
#  include <qoffscreensurface.h>
#endif

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(qLcEvrD3DPresentEngine, "qt.multimedia.evrd3dpresentengine");

class IMFSampleVideoBuffer : public QHwVideoBuffer
{
public:
    IMFSampleVideoBuffer(ComPtr<IDirect3DDevice9Ex> device, const ComPtr<IMFSample> &sample,
                         QRhi *rhi, QVideoFrame::HandleType type = QVideoFrame::NoHandle)
        : QHwVideoBuffer(type, rhi),
          m_device(device),
          m_sample(sample),
          m_mapMode(QVideoFrame::NotMapped)
    {
    }

    ~IMFSampleVideoBuffer() override
    {
        if (m_memSurface && m_mapMode != QVideoFrame::NotMapped)
            m_memSurface->UnlockRect();
    }

    MapData map(QVideoFrame::MapMode mode) override
    {
        if (!m_sample || m_mapMode != QVideoFrame::NotMapped || mode != QVideoFrame::ReadOnly)
            return {};

        D3DSURFACE_DESC desc;
        if (m_memSurface) {
            if (FAILED(m_memSurface->GetDesc(&desc)))
                return {};

        } else {
            ComPtr<IMFMediaBuffer> buffer;
            HRESULT hr = m_sample->GetBufferByIndex(0, buffer.GetAddressOf());
            if (FAILED(hr))
                return {};

            ComPtr<IDirect3DSurface9> surface;
            hr = MFGetService(buffer.Get(), MR_BUFFER_SERVICE, IID_IDirect3DSurface9, (void **)(surface.GetAddressOf()));
            if (FAILED(hr))
                return {};

            if (FAILED(surface->GetDesc(&desc)))
                return {};

            if (FAILED(m_device->CreateOffscreenPlainSurface(desc.Width, desc.Height, desc.Format, D3DPOOL_SYSTEMMEM, m_memSurface.GetAddressOf(), nullptr)))
                return {};

            if (FAILED(m_device->GetRenderTargetData(surface.Get(), m_memSurface.Get()))) {
                m_memSurface.Reset();
                return {};
            }
        }

        D3DLOCKED_RECT rect;
        if (FAILED(m_memSurface->LockRect(&rect, NULL, mode == QVideoFrame::ReadOnly ? D3DLOCK_READONLY : 0)))
            return {};

        m_mapMode = mode;

        MapData mapData;
        mapData.planeCount = 1;
        mapData.bytesPerLine[0] = (int)rect.Pitch;
        mapData.data[0] = reinterpret_cast<uchar *>(rect.pBits);
        mapData.dataSize[0] = (int)(rect.Pitch * desc.Height);
        return mapData;
    }

    void unmap() override
    {
        if (m_mapMode == QVideoFrame::NotMapped)
            return;

        m_mapMode = QVideoFrame::NotMapped;
        if (m_memSurface)
            m_memSurface->UnlockRect();
    }

protected:
    ComPtr<IDirect3DDevice9Ex> m_device;
    ComPtr<IMFSample> m_sample;

private:
    ComPtr<IDirect3DSurface9> m_memSurface;
    QVideoFrame::MapMode m_mapMode;
};

class QVideoFrameD3D11Textures: public QVideoFrameTextures
{
public:
    QVideoFrameD3D11Textures(std::unique_ptr<QRhiTexture> &&tex, ComPtr<ID3D11Texture2D> &&d3d11tex)
       : m_tex(std::move(tex))
       , m_d3d11tex(std::move(d3d11tex))
    {}

    QRhiTexture *texture(uint plane) const override
    {
        return plane == 0 ? m_tex.get() : nullptr;
    };

private:
    std::unique_ptr<QRhiTexture> m_tex;
    ComPtr<ID3D11Texture2D> m_d3d11tex;
};

class D3D11TextureVideoBuffer: public IMFSampleVideoBuffer
{
public:
    D3D11TextureVideoBuffer(ComPtr<IDirect3DDevice9Ex> device, const ComPtr<IMFSample> &sample,
                            HANDLE sharedHandle, QRhi *rhi)
        : IMFSampleVideoBuffer(std::move(device), sample, rhi, QVideoFrame::RhiTextureHandle)
        , m_sharedHandle(sharedHandle)
    {}

    QVideoFrameTexturesUPtr mapTextures(QRhi &rhi, QVideoFrameTexturesUPtr& /*oldTextures*/) override
    {
        if (rhi.backend() != QRhi::D3D11)
            return {};

        auto nh = static_cast<const QRhiD3D11NativeHandles*>(rhi.nativeHandles());
        if (!nh)
            return {};

        auto dev = reinterpret_cast<ID3D11Device *>(nh->dev);
        if (!dev)
            return {};

        ComPtr<ID3D11Texture2D> d3d11tex;
        HRESULT hr = dev->OpenSharedResource(m_sharedHandle, __uuidof(ID3D11Texture2D), (void**)(d3d11tex.GetAddressOf()));
        if (SUCCEEDED(hr)) {
            D3D11_TEXTURE2D_DESC desc = {};
            d3d11tex->GetDesc(&desc);
            QRhiTexture::Format format;
            if (desc.Format == DXGI_FORMAT_B8G8R8A8_UNORM)
                format = QRhiTexture::BGRA8;
            else if (desc.Format == DXGI_FORMAT_R8G8B8A8_UNORM)
                format = QRhiTexture::RGBA8;
            else
                return {};

            std::unique_ptr<QRhiTexture> tex(rhi.newTexture(format, QSize{int(desc.Width), int(desc.Height)}, 1, {}));
            tex->createFrom({quint64(d3d11tex.Get()), 0});
            return std::make_unique<QVideoFrameD3D11Textures>(std::move(tex), std::move(d3d11tex));

        } else {
            qCDebug(qLcEvrD3DPresentEngine) << "Failed to obtain D3D11Texture2D from D3D9Texture2D handle";
        }
        return {};
    }

private:
    HANDLE m_sharedHandle = nullptr;
};

#if QT_CONFIG(opengl)
class QVideoFrameOpenGlTextures : public QVideoFrameTextures
{
    struct InterOpHandles {
        GLuint textureName = 0;
        HANDLE device = nullptr;
        HANDLE texture = nullptr;
    };

public:
    Q_DISABLE_COPY(QVideoFrameOpenGlTextures);

    QVideoFrameOpenGlTextures(std::unique_ptr<QRhiTexture> &&tex, const WglNvDxInterop &wgl, InterOpHandles &handles)
        : m_tex(std::move(tex))
        , m_wgl(wgl)
        , m_handles(handles)
    {}

    ~QVideoFrameOpenGlTextures() override {
        if (QOpenGLContext::currentContext()) {
            if (!m_wgl.wglDXUnlockObjectsNV(m_handles.device, 1, &m_handles.texture))
                qCDebug(qLcEvrD3DPresentEngine) << "Failed to unlock OpenGL texture";

            if (!m_wgl.wglDXUnregisterObjectNV(m_handles.device, m_handles.texture))
                qCDebug(qLcEvrD3DPresentEngine) << "Failed to unregister OpenGL texture";

            QOpenGLFunctions *funcs = QOpenGLContext::currentContext()->functions();
            if (funcs)
                funcs->glDeleteTextures(1, &m_handles.textureName);
            else
                qCDebug(qLcEvrD3DPresentEngine) << "Could not delete texture, OpenGL context functions missing";

            if (!m_wgl.wglDXCloseDeviceNV(m_handles.device))
                qCDebug(qLcEvrD3DPresentEngine) << "Failed to close D3D-GL device";

        } else {
            qCDebug(qLcEvrD3DPresentEngine) << "Could not release texture, OpenGL context missing";
        }
    }

    static std::unique_ptr<QVideoFrameOpenGlTextures> create(const WglNvDxInterop &wgl, QRhi *rhi,
                                                             IDirect3DDevice9Ex *device, IDirect3DTexture9 *texture,
                                                             HANDLE sharedHandle)
    {
        if (!rhi || rhi->backend() != QRhi::OpenGLES2)
            return {};

        if (!QOpenGLContext::currentContext())
            return {};

        InterOpHandles handles = {};
        handles.device = wgl.wglDXOpenDeviceNV(device);
        if (!handles.device) {
            qCDebug(qLcEvrD3DPresentEngine) << "Failed to open D3D device";
            return {};
        }

        wgl.wglDXSetResourceShareHandleNV(texture, sharedHandle);

        QOpenGLFunctions *funcs = QOpenGLContext::currentContext()->functions();
        if (funcs) {
            funcs->glGenTextures(1, &handles.textureName);
            handles.texture = wgl.wglDXRegisterObjectNV(handles.device, texture, handles.textureName,
                                                        GL_TEXTURE_2D, WglNvDxInterop::WGL_ACCESS_READ_ONLY_NV);
            if (handles.texture) {
                if (wgl.wglDXLockObjectsNV(handles.device, 1, &handles.texture)) {
                    D3DSURFACE_DESC desc;
                    texture->GetLevelDesc(0, &desc);
                    QRhiTexture::Format format;
                    if (desc.Format == D3DFMT_A8R8G8B8)
                        format = QRhiTexture::BGRA8;
                    else if (desc.Format == D3DFMT_A8B8G8R8)
                        format = QRhiTexture::RGBA8;
                    else
                        return {};

                    std::unique_ptr<QRhiTexture> tex(rhi->newTexture(format, QSize{int(desc.Width), int(desc.Height)}, 1, {}));
                    tex->createFrom({quint64(handles.textureName), 0});
                    return std::make_unique<QVideoFrameOpenGlTextures>(std::move(tex), wgl, handles);
                }

                qCDebug(qLcEvrD3DPresentEngine) << "Failed to lock OpenGL texture";
                wgl.wglDXUnregisterObjectNV(handles.device, handles.texture);
            } else {
                qCDebug(qLcEvrD3DPresentEngine) << "Could not register D3D9 texture in OpenGL";
            }

            funcs->glDeleteTextures(1, &handles.textureName);
        } else {
            qCDebug(qLcEvrD3DPresentEngine) << "Failed generate texture names, OpenGL context functions missing";
        }
        return {};
    }

    QRhiTexture *texture(uint plane) const override
    {
        return plane == 0 ? m_tex.get() : nullptr;
    };
private:
    std::unique_ptr<QRhiTexture> m_tex;
    WglNvDxInterop m_wgl;
    InterOpHandles m_handles;
};

class OpenGlVideoBuffer: public IMFSampleVideoBuffer
{
public:
    OpenGlVideoBuffer(ComPtr<IDirect3DDevice9Ex> device, const ComPtr<IMFSample> &sample,
                      const WglNvDxInterop &wglNvDxInterop, HANDLE sharedHandle, QRhi *rhi)
        : IMFSampleVideoBuffer(std::move(device), sample, rhi, QVideoFrame::RhiTextureHandle)
        , m_sharedHandle(sharedHandle)
        , m_wgl(wglNvDxInterop)
    {}

    QVideoFrameTexturesUPtr mapTextures(QRhi &rhi, QVideoFrameTexturesUPtr& /*oldTextures*/) override
    {
        if (!m_texture) {
            ComPtr<IMFMediaBuffer> buffer;
            HRESULT hr = m_sample->GetBufferByIndex(0, buffer.GetAddressOf());
            if (FAILED(hr))
                return {};

            ComPtr<IDirect3DSurface9> surface;
            hr = MFGetService(buffer.Get(), MR_BUFFER_SERVICE, IID_IDirect3DSurface9,
                              (void **)(surface.GetAddressOf()));
            if (FAILED(hr))
                return {};

            hr = surface->GetContainer(IID_IDirect3DTexture9, (void **)m_texture.GetAddressOf());
            if (FAILED(hr))
                return {};
        }

        return QVideoFrameOpenGlTextures::create(m_wgl, &rhi, m_device.Get(), m_texture.Get(), m_sharedHandle);
    }

private:
    HANDLE m_sharedHandle = nullptr;
    WglNvDxInterop m_wgl;
    ComPtr<IDirect3DTexture9> m_texture;
};
#endif

D3DPresentEngine::D3DPresentEngine(QVideoSink *sink)
    : m_deviceResetToken(0)
{
    ZeroMemory(&m_displayMode, sizeof(m_displayMode));
    setSink(sink);
}

D3DPresentEngine::~D3DPresentEngine()
{
    releaseResources();
}

void D3DPresentEngine::setSink(QVideoSink *sink)
{
    if (sink == m_sink)
        return;

    m_sink = sink;

    releaseResources();
    m_device.Reset();
    m_devices.Reset();
    m_D3D9.Reset();

    if (!m_sink)
        return;

    HRESULT hr = initializeD3D();

    if (SUCCEEDED(hr)) {
        hr = createD3DDevice();
        if (FAILED(hr))
            qWarning("Failed to create D3D device");
    } else {
        qWarning("Failed to initialize D3D");
    }
}

HRESULT D3DPresentEngine::initializeD3D()
{
    HRESULT hr = Direct3DCreate9Ex(D3D_SDK_VERSION, m_D3D9.GetAddressOf());

    if (SUCCEEDED(hr))
        hr = DXVA2CreateDirect3DDeviceManager9(&m_deviceResetToken, m_devices.GetAddressOf());

    return hr;
}

static bool findD3D11AdapterID(QRhi &rhi, IDirect3D9Ex *D3D9, UINT &adapterID)
{
    auto nh = static_cast<const QRhiD3D11NativeHandles*>(rhi.nativeHandles());
    if (D3D9 && nh) {
        for (auto i = 0u; i < D3D9->GetAdapterCount(); ++i) {
            LUID luid = {};
            D3D9->GetAdapterLUID(i, &luid);
            if (luid.LowPart == nh->adapterLuidLow && luid.HighPart == nh->adapterLuidHigh) {
                adapterID = i;
                return true;
            }
        }
    }

    return false;
}

#if QT_CONFIG(opengl)
template <typename T>
static bool getProc(const QOpenGLContext *ctx, T &fn, const char *fName)
{
    fn = reinterpret_cast<T>(ctx->getProcAddress(fName));
    return fn != nullptr;
}

static bool readWglNvDxInteropProc(WglNvDxInterop &f)
{
    QScopedPointer<QOffscreenSurface> surface(new QOffscreenSurface);
    surface->create();
    QScopedPointer<QOpenGLContext> ctx(new QOpenGLContext);
    ctx->create();
    ctx->makeCurrent(surface.get());

    auto wglGetExtensionsStringARB = reinterpret_cast<const char* (WINAPI* )(HDC)>
            (ctx->getProcAddress("wglGetExtensionsStringARB"));
    if (!wglGetExtensionsStringARB) {
        qCDebug(qLcEvrD3DPresentEngine) << "WGL extensions missing (no wglGetExtensionsStringARB function)";
        return false;
    }

    HWND hwnd = ::GetShellWindow();
    auto dc = ::GetDC(hwnd);

    const char *wglExtString = wglGetExtensionsStringARB(dc);
    if (!wglExtString)
        qCDebug(qLcEvrD3DPresentEngine) << "WGL extensions missing (wglGetExtensionsStringARB returned null)";

    bool hasExtension = wglExtString && strstr(wglExtString, "WGL_NV_DX_interop");
    ReleaseDC(hwnd, dc);
    if (!hasExtension) {
        qCDebug(qLcEvrD3DPresentEngine) << "WGL_NV_DX_interop missing";
        return false;
    }

    return getProc(ctx.get(), f.wglDXOpenDeviceNV, "wglDXOpenDeviceNV")
        && getProc(ctx.get(), f.wglDXCloseDeviceNV, "wglDXCloseDeviceNV")
        && getProc(ctx.get(), f.wglDXSetResourceShareHandleNV, "wglDXSetResourceShareHandleNV")
        && getProc(ctx.get(), f.wglDXRegisterObjectNV, "wglDXRegisterObjectNV")
        && getProc(ctx.get(), f.wglDXUnregisterObjectNV, "wglDXUnregisterObjectNV")
        && getProc(ctx.get(), f.wglDXLockObjectsNV, "wglDXLockObjectsNV")
        && getProc(ctx.get(), f.wglDXUnlockObjectsNV, "wglDXUnlockObjectsNV");
}
#endif

namespace {

bool hwTextureRenderingEnabled() {
    // add possibility for an user to opt-out HW video rendering
    // using the same env. variable as for FFmpeg backend
    static bool isDisableConversionSet = false;
    static const int disableHwConversion = qEnvironmentVariableIntValue(
            "QT_DISABLE_HW_TEXTURES_CONVERSION", &isDisableConversionSet);

    return !isDisableConversionSet || !disableHwConversion;
}

}

HRESULT D3DPresentEngine::createD3DDevice()
{
    if (!m_D3D9 || !m_devices)
        return MF_E_NOT_INITIALIZED;

    m_useTextureRendering = false;
    UINT adapterID = 0;

    if (hwTextureRenderingEnabled()) {
        QRhi *rhi = m_sink ? m_sink->rhi() : nullptr;
        if (rhi) {
            if (rhi->backend() == QRhi::D3D11) {
                m_useTextureRendering = findD3D11AdapterID(*rhi, m_D3D9.Get(), adapterID);
#if QT_CONFIG(opengl)
            } else if (rhi->backend() == QRhi::OpenGLES2)  {
                m_useTextureRendering = readWglNvDxInteropProc(m_wglNvDxInterop);
#endif
            } else {
                qCDebug(qLcEvrD3DPresentEngine) << "Not supported RHI backend type";
            }
        } else {
            qCDebug(qLcEvrD3DPresentEngine) << "No RHI associated with this sink";
        }

        if (!m_useTextureRendering)
            qCDebug(qLcEvrD3DPresentEngine) << "Could not find compatible RHI adapter, zero copy disabled";
    }

    D3DCAPS9 ddCaps;
    ZeroMemory(&ddCaps, sizeof(ddCaps));

    HRESULT hr = m_D3D9->GetDeviceCaps(adapterID, D3DDEVTYPE_HAL, &ddCaps);
    if (FAILED(hr))
        return hr;

    DWORD vp = 0;
    if (ddCaps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT)
        vp = D3DCREATE_HARDWARE_VERTEXPROCESSING;
    else
        vp = D3DCREATE_SOFTWARE_VERTEXPROCESSING;

    D3DPRESENT_PARAMETERS pp;
    ZeroMemory(&pp, sizeof(pp));

    pp.BackBufferWidth = 1;
    pp.BackBufferHeight = 1;
    pp.BackBufferCount = 1;
    pp.Windowed = TRUE;
    pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    pp.BackBufferFormat = D3DFMT_UNKNOWN;
    pp.hDeviceWindow = nullptr;
    pp.Flags = D3DPRESENTFLAG_VIDEO;
    pp.PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;

    ComPtr<IDirect3DDevice9Ex> device;

    hr = m_D3D9->CreateDeviceEx(
                adapterID,
                D3DDEVTYPE_HAL,
                pp.hDeviceWindow,
                vp | D3DCREATE_NOWINDOWCHANGES | D3DCREATE_MULTITHREADED | D3DCREATE_FPU_PRESERVE,
                &pp,
                NULL,
                device.GetAddressOf()
                );
    if (FAILED(hr))
        return hr;

    hr = m_D3D9->GetAdapterDisplayMode(adapterID, &m_displayMode);
    if (FAILED(hr))
        return hr;

    hr = m_devices->ResetDevice(device.Get(), m_deviceResetToken);
    if (FAILED(hr))
        return hr;

    m_device = device;
    return hr;
}

bool D3DPresentEngine::isValid() const
{
    return m_device.Get() != nullptr;
}

void D3DPresentEngine::releaseResources()
{
    m_surfaceFormat = QVideoFrameFormat();
}

HRESULT D3DPresentEngine::getService(REFGUID, REFIID riid, void** ppv)
{
    HRESULT hr = S_OK;

    if (riid == __uuidof(IDirect3DDeviceManager9)) {
        if (!m_devices) {
            hr = MF_E_UNSUPPORTED_SERVICE;
        } else {
            *ppv = m_devices.Get();
            m_devices->AddRef();
        }
    } else {
        hr = MF_E_UNSUPPORTED_SERVICE;
    }

    return hr;
}

HRESULT D3DPresentEngine::checkFormat(D3DFORMAT format)
{
    if (!m_D3D9 || !m_device)
        return E_FAIL;

    HRESULT hr = S_OK;

    D3DDISPLAYMODE mode;
    D3DDEVICE_CREATION_PARAMETERS params;

    hr = m_device->GetCreationParameters(&params);
    if (FAILED(hr))
        return hr;

    UINT uAdapter = params.AdapterOrdinal;
    D3DDEVTYPE type = params.DeviceType;

    hr = m_D3D9->GetAdapterDisplayMode(uAdapter, &mode);
    if (FAILED(hr))
        return hr;

    hr = m_D3D9->CheckDeviceFormat(uAdapter, type, mode.Format,
                                   D3DUSAGE_RENDERTARGET,
                                   D3DRTYPE_SURFACE,
                                   format);
    if (FAILED(hr))
        return hr;

    bool ok = format == D3DFMT_X8R8G8B8
           || format == D3DFMT_A8R8G8B8
           || format == D3DFMT_X8B8G8R8
           || format == D3DFMT_A8B8G8R8;

    return ok ? S_OK : D3DERR_NOTAVAILABLE;
}

HRESULT D3DPresentEngine::createVideoSamples(IMFMediaType *format,
                                             QList<ComPtr<IMFSample>> &videoSampleQueue,
                                             QSize frameSize)
{
    if (!format || !m_device)
        return MF_E_UNEXPECTED;

    HRESULT hr = S_OK;
    releaseResources();

    UINT32 width = 0, height = 0;
    hr = MFGetAttributeSize(format, MF_MT_FRAME_SIZE, &width, &height);
    if (FAILED(hr))
        return hr;

    if (frameSize.isValid() && !frameSize.isEmpty()) {
        width = frameSize.width();
        height = frameSize.height();
    }

    DWORD d3dFormat = 0;
    hr = qt_evr_getFourCC(format, &d3dFormat);
    if (FAILED(hr))
        return hr;

    // FIXME: RHI defines only RGBA, thus add the alpha channel to the selected format
    if (d3dFormat == D3DFMT_X8R8G8B8)
        d3dFormat = D3DFMT_A8R8G8B8;
    else if (d3dFormat == D3DFMT_X8B8G8R8)
        d3dFormat = D3DFMT_A8B8G8R8;

    for (int i = 0; i < PRESENTER_BUFFER_COUNT; i++) {
        // texture ref cnt is increased by GetSurfaceLevel()/MFCreateVideoSampleFromSurface()
        // below, so it will be destroyed only when the sample pool is released.
        ComPtr<IDirect3DTexture9> texture;
        HANDLE sharedHandle = nullptr;
        hr = m_device->CreateTexture(width, height, 1, D3DUSAGE_RENDERTARGET, (D3DFORMAT)d3dFormat,  D3DPOOL_DEFAULT, texture.GetAddressOf(), &sharedHandle);
        if (FAILED(hr))
            break;

        ComPtr<IDirect3DSurface9> surface;
        hr = texture->GetSurfaceLevel(0, surface.GetAddressOf());
        if (FAILED(hr))
            break;

        ComPtr<IMFSample> videoSample;
        hr = MFCreateVideoSampleFromSurface(surface.Get(), videoSample.GetAddressOf());
        if (FAILED(hr))
            break;

        m_sampleTextureHandle[i] = {videoSample.Get(), sharedHandle};
        videoSampleQueue.append(videoSample);
    }

    if (SUCCEEDED(hr)) {
        m_surfaceFormat = QVideoFrameFormat(QSize(width, height), qt_evr_pixelFormatFromD3DFormat(d3dFormat));
    } else {
        releaseResources();
    }

    return hr;
}

QVideoFrame D3DPresentEngine::makeVideoFrame(const ComPtr<IMFSample> &sample,
                                             QtVideo::Rotation rotation)
{
    if (!sample)
        return {};

    HANDLE sharedHandle = nullptr;
    for (const auto &p : m_sampleTextureHandle)
        if (p.first == sample.Get())
            sharedHandle = p.second;

    std::unique_ptr<IMFSampleVideoBuffer> vb;
    QRhi *rhi = m_sink ? m_sink->rhi() : nullptr;
    if (m_useTextureRendering && sharedHandle && rhi) {
        if (rhi->backend() == QRhi::D3D11) {
            vb = std::make_unique<D3D11TextureVideoBuffer>(m_device, sample, sharedHandle, rhi);
#if QT_CONFIG(opengl)
        } else if (rhi->backend() == QRhi::OpenGLES2) {
            vb = std::make_unique<OpenGlVideoBuffer>(m_device, sample, m_wglNvDxInterop,
                                                     sharedHandle, rhi);
#endif
        }
    }

    if (!vb)
        vb = std::make_unique<IMFSampleVideoBuffer>(m_device, sample, rhi);

    auto format = m_surfaceFormat;
    format.setRotation(rotation);
    QVideoFrame frame = QVideoFramePrivate::createFrame(std::move(vb), format);

    // WMF uses 100-nanosecond units, Qt uses microseconds
    LONGLONG startTime = 0;
    auto hr = sample->GetSampleTime(&startTime);
     if (SUCCEEDED(hr)) {
        frame.setStartTime(startTime / 10);

        LONGLONG duration = -1;
        if (SUCCEEDED(sample->GetSampleDuration(&duration)))
            frame.setEndTime((startTime + duration) / 10);
    }

    return frame;
}

QT_END_NAMESPACE
