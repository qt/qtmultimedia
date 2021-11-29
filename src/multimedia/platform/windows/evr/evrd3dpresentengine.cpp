/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "evrd3dpresentengine_p.h"

#include "evrhelpers_p.h"

#include <private/qabstractvideobuffer_p.h>
#include <qvideoframe.h>
#include <QDebug>
#include <qthread.h>
#include <qvideosink.h>
#include <qloggingcategory.h>

#include <d3d11_1.h>

#include <private/qrhi_p.h>
#include <private/qrhid3d11_p.h>


QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(qLcEvrD3DPresentEngine, "qt.multimedia.evrd3dpresentengine")

class IMFSampleVideoBuffer: public QAbstractVideoBuffer
{
public:
    IMFSampleVideoBuffer(QWindowsIUPointer<IDirect3DDevice9Ex> device,
                          IMFSample *sample,
                          QWindowsIUPointer<ID3D11Texture2D> d2d11tex)
        : QAbstractVideoBuffer(d2d11tex ? QVideoFrame::RhiTextureHandle : QVideoFrame::NoHandle)
        , m_device(device)
        , m_d2d11tex(d2d11tex)
        , m_mapMode(QVideoFrame::NotMapped)
    {
        sample->AddRef();
        m_sample.reset(sample);
    }

    ~IMFSampleVideoBuffer() override
    {
        if (m_memSurface && m_mapMode != QVideoFrame::NotMapped)
            m_memSurface->UnlockRect();
    }

    QVideoFrame::MapMode mapMode() const override { return m_mapMode; }
    MapData map(QVideoFrame::MapMode mode) override;
    void unmap() override;

    void mapTextures() override {}
    quint64 textureHandle(int /*plane*/) const override { return quint64(m_d2d11tex.get()); }

private:
    QWindowsIUPointer<IDirect3DDevice9Ex> m_device;
    QWindowsIUPointer<ID3D11Texture2D> m_d2d11tex;
    QWindowsIUPointer<IMFSample> m_sample;
    QWindowsIUPointer<IDirect3DSurface9> m_memSurface;
    QVideoFrame::MapMode m_mapMode;
};

IMFSampleVideoBuffer::MapData IMFSampleVideoBuffer::map(QVideoFrame::MapMode mode)
{
    if (!m_sample || m_mapMode != QVideoFrame::NotMapped || mode != QVideoFrame::ReadOnly)
        return {};

    D3DSURFACE_DESC desc;
    if (m_memSurface) {
        if (FAILED(m_memSurface->GetDesc(&desc)))
            return {};

    } else {
        QWindowsIUPointer<IMFMediaBuffer> buffer;
        HRESULT hr = m_sample->GetBufferByIndex(0, buffer.address());
        if (FAILED(hr))
            return {};

        QWindowsIUPointer<IDirect3DSurface9> surface;
        hr = MFGetService(buffer.get(), MR_BUFFER_SERVICE, IID_IDirect3DSurface9, (void **)(surface.address()));
        if (FAILED(hr))
            return {};

        if (FAILED(surface->GetDesc(&desc)))
            return {};

        if (FAILED(m_device->CreateOffscreenPlainSurface(desc.Width, desc.Height, desc.Format, D3DPOOL_SYSTEMMEM, m_memSurface.address(), nullptr)))
            return {};

        if (FAILED(m_device->GetRenderTargetData(surface.get(), m_memSurface.get()))) {
            m_memSurface.reset();
            return {};
        }
    }

    D3DLOCKED_RECT rect;
    if (FAILED(m_memSurface->LockRect(&rect, NULL, mode == QVideoFrame::ReadOnly ? D3DLOCK_READONLY : 0)))
        return {};

    m_mapMode = mode;

    MapData mapData;
    mapData.nPlanes = 1;
    mapData.bytesPerLine[0] = (int)rect.Pitch;
    mapData.data[0] = reinterpret_cast<uchar *>(rect.pBits);
    mapData.size[0] = (int)(rect.Pitch * desc.Height);
    return mapData;
}

void IMFSampleVideoBuffer::unmap()
{
    if (m_mapMode == QVideoFrame::NotMapped)
        return;

    m_mapMode = QVideoFrame::NotMapped;
    if (m_memSurface)
        m_memSurface->UnlockRect();
}

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
    m_device.reset();
    m_devices.reset();
    m_D3D9.reset();

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
    HRESULT hr = Direct3DCreate9Ex(D3D_SDK_VERSION, m_D3D9.address());

    if (SUCCEEDED(hr))
        hr = DXVA2CreateDirect3DDeviceManager9(&m_deviceResetToken, m_devices.address());

    return hr;
}

HRESULT D3DPresentEngine::createD3DDevice()
{
    if (!m_D3D9 || !m_devices)
        return MF_E_NOT_INITIALIZED;

    m_useTextureRendering = false;
    UINT adapterID = 0;
    if (m_sink->rhi()) {
        if (m_sink->rhi()->backend() == QRhi::D3D11) {
            auto nh = static_cast<const QRhiD3D11NativeHandles*>(m_sink->rhi()->nativeHandles());
            if (nh) {
                for (auto i = 0u; i < m_D3D9->GetAdapterCount(); ++i) {
                    LUID luid = {};
                    m_D3D9->GetAdapterLUID(i, &luid);
                    if (luid.LowPart == nh->adapterLuidLow && luid.HighPart == nh->adapterLuidHigh) {
                        adapterID = i;
                        m_useTextureRendering = true;
                        break;
                    }
                }
            }
        } else {
            qCDebug(qLcEvrD3DPresentEngine) << "Not supported RHI backend type";
        }
    } else {
        qCDebug(qLcEvrD3DPresentEngine) << "No RHI associated with this sink";
        adapterID = 0;
    }

    if (!m_useTextureRendering)
        qCDebug(qLcEvrD3DPresentEngine) << "Could not find compatible RHI adapter, zero copy disabled";

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

    QWindowsIUPointer<IDirect3DDevice9Ex> device;

    hr = m_D3D9->CreateDeviceEx(
                adapterID,
                D3DDEVTYPE_HAL,
                pp.hDeviceWindow,
                vp | D3DCREATE_NOWINDOWCHANGES | D3DCREATE_MULTITHREADED | D3DCREATE_FPU_PRESERVE,
                &pp,
                NULL,
                device.address()
                );
    if (FAILED(hr))
        return hr;

    hr = m_D3D9->GetAdapterDisplayMode(adapterID, &m_displayMode);
    if (FAILED(hr))
        return hr;

    hr = m_devices->ResetDevice(device.get(), m_deviceResetToken);
    if (FAILED(hr))
        return hr;

    m_device = device;
    return hr;
}

bool D3DPresentEngine::isValid() const
{
    return m_device.get() != nullptr;
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
            *ppv = m_devices.get();
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

    return hr;
}

HRESULT D3DPresentEngine::createVideoSamples(IMFMediaType *format, QList<IMFSample*> &videoSampleQueue)
{
    if (!format || !m_device)
        return MF_E_UNEXPECTED;

    HRESULT hr = S_OK;
    releaseResources();

    UINT32 width = 0, height = 0;
    hr = MFGetAttributeSize(format, MF_MT_FRAME_SIZE, &width, &height);
    if (FAILED(hr))
        return hr;

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
        IDirect3DTexture9 *texture = nullptr;
        HANDLE sharedHandle = nullptr;
        hr = m_device->CreateTexture(width, height, 1, D3DUSAGE_RENDERTARGET, (D3DFORMAT)d3dFormat,  D3DPOOL_DEFAULT, &texture, &sharedHandle);
        if (FAILED(hr))
            break;

        QWindowsIUPointer<IDirect3DSurface9> surface;
        hr = texture->GetSurfaceLevel(0, surface.address());
        if (FAILED(hr))
            break;

        QWindowsIUPointer<IMFSample> videoSample;
        hr = MFCreateVideoSampleFromSurface(surface.get(), videoSample.address());
        if (FAILED(hr))
            break;

        m_sampleTextureHandle[i] = {videoSample.get(), sharedHandle};
        videoSampleQueue.append(videoSample.release());
    }

    if (SUCCEEDED(hr)) {
        m_surfaceFormat = QVideoFrameFormat(QSize(width, height), qt_evr_pixelFormatFromD3DFormat(d3dFormat));
    } else {
        releaseResources();
    }

    return hr;
}

QVideoFrame D3DPresentEngine::makeVideoFrame(IMFSample *sample)
{
    if (!sample)
        return {};

    HANDLE sharedHandle = nullptr;
    for (const auto &p : m_sampleTextureHandle)
        if (p.first == sample)
            sharedHandle = p.second;

    QWindowsIUPointer<ID3D11Texture2D> d3d11tex;
    if (m_useTextureRendering && sharedHandle && m_sink->rhi()) {
        auto nh = static_cast<const QRhiD3D11NativeHandles*>(m_sink->rhi()->nativeHandles());
        if (nh) {
            auto dev = reinterpret_cast<ID3D11Device *>(nh->dev);
            if (dev) {
                HRESULT hr = dev->OpenSharedResource(sharedHandle, __uuidof(ID3D11Texture2D), (void**)(d3d11tex.address()));
                if (FAILED(hr))
                    qWarning() << "Failed to obtain D3D11Texture2D from D3D9Texture2D handle";
            }
        }
    }

    QVideoFrame frame(new IMFSampleVideoBuffer(m_device, sample, d3d11tex), m_surfaceFormat);

    // WMF uses 100-nanosecond units, Qt uses microseconds
    LONGLONG startTime = 0;
    auto hr = sample->GetSampleTime(&startTime);
     if (SUCCEEDED(hr)) {
        frame.setStartTime(startTime * 0.1);

        LONGLONG duration = -1;
        if (SUCCEEDED(sample->GetSampleDuration(&duration)))
            frame.setEndTime((startTime + duration) * 0.1);
    }

    return frame;
}

QT_END_NAMESPACE
