// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR
// GPL-3.0-only

#include "qffmpegscreencapture_dxgi_p.h"
#include "qffmpegscreencapturethread_p.h"
#include <private/qabstractvideobuffer_p.h>
#include <private/qmultimediautils_p.h>
#include <private/qwindowsmultimediautils_p.h>
#include <qpa/qplatformscreen_p.h>
#include "qvideoframe.h"

#include <qguiapplication.h>
#include <qloggingcategory.h>
#include <qthread.h>
#include <qwaitcondition.h>
#include <qmutex.h>

#include "D3d11.h"
#include "dxgi1_2.h"
#include <system_error>
#include <thread>
#include <chrono>

QT_BEGIN_NAMESPACE

static Q_LOGGING_CATEGORY(qLcScreenCaptureDxgi, "qt.multimedia.ffmpeg.screencapturedxgi")

using namespace std::chrono;
using namespace QWindowsMultimediaUtils;

class QD3D11TextureVideoBuffer : public QAbstractVideoBuffer
{
public:
    QD3D11TextureVideoBuffer(ComPtr<ID3D11Device> &device, std::shared_ptr<QMutex> &mutex,
                             ComPtr<ID3D11Texture2D> &texture, QSize size)
        : QAbstractVideoBuffer(QVideoFrame::NoHandle)
        , m_device(device)
        , m_texture(texture)
        , m_ctxMutex(mutex)
        , m_size(size)
    {}

    ~QD3D11TextureVideoBuffer()
    {
        QD3D11TextureVideoBuffer::unmap();
    }

    QVideoFrame::MapMode mapMode() const override
    {
        return m_mapMode;
    }

    MapData map(QVideoFrame::MapMode mode) override
    {
        MapData mapData;
        if (!m_ctx && mode == QVideoFrame::ReadOnly) {
            D3D11_TEXTURE2D_DESC texDesc = {};
            m_texture->GetDesc(&texDesc);
            texDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
            texDesc.Usage = D3D11_USAGE_STAGING;
            texDesc.MiscFlags = 0;
            texDesc.BindFlags = 0;

            HRESULT hr = m_device->CreateTexture2D(&texDesc, nullptr, m_cpuTexture.GetAddressOf());
            if (FAILED(hr)) {
                qCDebug(qLcScreenCaptureDxgi) << "Failed to create texture with CPU access"
                                              << std::system_category().message(hr).c_str();
                qCDebug(qLcScreenCaptureDxgi) << m_device->GetDeviceRemovedReason();
                return {};
            }

            m_device->GetImmediateContext(m_ctx.GetAddressOf());
            m_ctxMutex->lock();
            m_ctx->CopyResource(m_cpuTexture.Get(), m_texture.Get());

            D3D11_MAPPED_SUBRESOURCE resource = {};
            hr = m_ctx->Map(m_cpuTexture.Get(), 0, D3D11_MAP_READ, 0, &resource);
            m_ctxMutex->unlock();
            if (FAILED(hr)) {
                qCDebug(qLcScreenCaptureDxgi) << "Failed to map texture" << m_cpuTexture.Get()
                                              << std::system_category().message(hr).c_str();
                return {};
            }

            m_mapMode = mode;
            mapData.nPlanes = 1;
            mapData.bytesPerLine[0] = int(resource.RowPitch);
            mapData.data[0] = reinterpret_cast<uchar*>(resource.pData);
            mapData.size[0] = m_size.height() * int(resource.RowPitch);
        }

        return mapData;
    }

    void unmap() override
    {
        if (m_mapMode == QVideoFrame::NotMapped)
            return;
        if (m_ctx) {
            m_ctxMutex->lock();
            m_ctx->Unmap(m_cpuTexture.Get(), 0);
            m_ctxMutex->unlock();
            m_ctx.Reset();
        }
        m_cpuTexture.Reset();
        m_mapMode = QVideoFrame::NotMapped;
    }

private:
    ComPtr<ID3D11Device> m_device;
    ComPtr<ID3D11Texture2D> m_texture;
    ComPtr<ID3D11Texture2D> m_cpuTexture;
    ComPtr<ID3D11DeviceContext> m_ctx;
    std::shared_ptr<QMutex> m_ctxMutex;
    QSize m_size;
    QVideoFrame::MapMode m_mapMode = QVideoFrame::NotMapped;
};

class QFFmpegScreenCaptureDxgi::Grabber : public QFFmpegScreenCaptureThread
{
public:
    Grabber(QFFmpegScreenCaptureDxgi &screenCapture, QScreen *screen, ComPtr<ID3D11Device> &device,
            ComPtr<IDXGIOutputDuplication> &duplication)
        : QFFmpegScreenCaptureThread()
        , m_duplication(duplication)
        , m_device(device)
        , m_ctxMutex(std::make_shared<QMutex>())
    {
        setFrameRate(screen->refreshRate());
        addFrameCallback(screenCapture, &QFFmpegScreenCaptureDxgi::newVideoFrame);
        connect(this, &Grabber::errorUpdated, &screenCapture, &QFFmpegScreenCaptureDxgi::updateError);
    }

    ~Grabber() {
        stop();
        if (m_releaseFrame)
            m_duplication->ReleaseFrame();
    }

    void run() override
    {
        DXGI_OUTDUPL_DESC outputDesc = {};
        m_duplication->GetDesc(&outputDesc);

        m_frameSize = { int(outputDesc.ModeDesc.Width), int(outputDesc.ModeDesc.Height) };
        QVideoFrameFormat frameFormat(m_frameSize, QVideoFrameFormat::Format_BGRA8888);

        m_formatMutex.lock();
        m_format = frameFormat;
        m_format.setFrameRate(int(frameRate()));
        m_formatMutex.unlock();
        m_waitForFormat.wakeAll();

        QFFmpegScreenCaptureThread::run();
    }

    QVideoFrameFormat format() {
        QMutexLocker locker(&m_formatMutex);
        if (!m_format.isValid())
            m_waitForFormat.wait(&m_formatMutex);
        return m_format;
    }

    QVideoFrame grabFrame() override {
        m_ctxMutex->lock();
        auto maybeTex = getNextFrame();
        m_ctxMutex->unlock();

        if (maybeTex) {
            auto buffer = new QD3D11TextureVideoBuffer(m_device, m_ctxMutex, maybeTex.value(), m_frameSize);
            return QVideoFrame(buffer, format());
        } else {
            const auto status = maybeTex.error().isEmpty() ? QScreenCapture::NoError
                                                           : QScreenCapture::CaptureFailed;
            updateError(status, maybeTex.error());
        }
        return {};
    };

private:
    QMaybe<ComPtr<ID3D11Texture2D>> getNextFrame()
    {
        if (m_releaseFrame) {
            m_releaseFrame = false;

            HRESULT hr = m_duplication->ReleaseFrame();
            if (FAILED(hr))
                return "Failed to release duplication frame. " + ::errorString(hr);
        }

        ComPtr<IDXGIResource> frame;
        DXGI_OUTDUPL_FRAME_INFO info;
        HRESULT hr = m_duplication->AcquireNextFrame(0, &info, frame.GetAddressOf());
        if (FAILED(hr))
            return hr == DXGI_ERROR_WAIT_TIMEOUT ? QString{}
                                                 : "Failed to grab the screen content" + ::errorString(hr);

        m_releaseFrame = true;

        ComPtr<ID3D11Texture2D> tex;
        hr = frame->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void **>(tex.GetAddressOf()));
        if (FAILED(hr))
            return "Failed to obtain D3D11 texture" + ::errorString(hr);

        D3D11_TEXTURE2D_DESC texDesc = {};
        tex->GetDesc(&texDesc);
        texDesc.MiscFlags = 0;
        texDesc.BindFlags = 0;
        ComPtr<ID3D11Texture2D> texCopy;
        hr = m_device->CreateTexture2D(&texDesc, nullptr, texCopy.GetAddressOf());
        if (FAILED(hr))
            return "Failed to create texture with CPU access" + ::errorString(hr);

        ComPtr<ID3D11DeviceContext> ctx;
        m_device->GetImmediateContext(ctx.GetAddressOf());
        ctx->CopyResource(texCopy.Get(), tex.Get());

        return texCopy;
    }

    ComPtr<IDXGIOutputDuplication> m_duplication;
    ComPtr<ID3D11Device> m_device;
    QWaitCondition m_waitForFormat;
    QVideoFrameFormat m_format;
    QMutex m_formatMutex;
    std::shared_ptr<QMutex> m_ctxMutex;
    QSize m_frameSize;
    bool m_releaseFrame = false;
};

static QMaybe<ComPtr<IDXGIOutputDuplication>> duplicateOutput(ID3D11Device* device, IDXGIOutput *output)
{
    ComPtr<IDXGIOutput1> output1;
    HRESULT hr = output->QueryInterface(__uuidof(IDXGIOutput1), reinterpret_cast<void**>(output1.GetAddressOf()));
    if (FAILED(hr))
        return  { "Failed to create IDXGIOutput1" + QString(std::system_category().message(hr).c_str()) };

    ComPtr<IDXGIOutputDuplication> dup;
    hr = output1->DuplicateOutput(device, dup.GetAddressOf());
    if (SUCCEEDED(hr))
        return dup;
    else
        return { "Failed to duplicate IDXGIOutput1" + QString(std::system_category().message(hr).c_str()) };
}

struct DxgiScreen {
    ComPtr<IDXGIAdapter1> adapter;
    ComPtr<IDXGIOutput> output;
};

static QMaybe<DxgiScreen> findDxgiScreen(const QScreen *screen)
{
    if (!screen)
        return QString("Cannot find nullptr screen");

    auto *winScreen = screen->nativeInterface<QNativeInterface::Private::QWindowsScreen>();
    HMONITOR handle = winScreen ? winScreen->handle() : nullptr;

    ComPtr<IDXGIFactory1> factory;
    HRESULT hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(factory.GetAddressOf()));
    if (FAILED(hr))
        return "Failed to create IDXGIFactory" + errorString(hr);

    ComPtr<IDXGIAdapter1> adapter;
    for (quint32 i = 0; SUCCEEDED(factory->EnumAdapters1(i, adapter.GetAddressOf())); i++, adapter.Reset()) {
        ComPtr<IDXGIOutput> output;
        for (quint32 j = 0; SUCCEEDED(adapter->EnumOutputs(j, output.GetAddressOf())); j++, output.Reset()) {
            DXGI_OUTPUT_DESC desc = {};
            output->GetDesc(&desc);
            qCDebug(qLcScreenCaptureDxgi) << i << j << QString::fromWCharArray(desc.DeviceName);
            auto match = handle ? handle == desc.Monitor
                                : QString::fromWCharArray(desc.DeviceName) == screen->name();
            if (match)
                return DxgiScreen{ adapter, output };
        }
    }
    return "Could not find screen adapter" + screen->name();
}

static QMaybe<ComPtr<ID3D11Device>> createD3D11Device(IDXGIAdapter1 *adapter)
{
    ComPtr<ID3D11Device> d3d11dev;
    HRESULT hr = D3D11CreateDevice(adapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr,
                                   0, nullptr, 0, D3D11_SDK_VERSION,
                                   d3d11dev.GetAddressOf(), nullptr, nullptr);
    if (SUCCEEDED(hr))
        return d3d11dev;
    else
        return "Failed to create ID3D11Device device" + errorString(hr);
}

QFFmpegScreenCaptureDxgi::QFFmpegScreenCaptureDxgi(QScreenCapture *screenCapture)
    : QFFmpegScreenCaptureBase(screenCapture)
{
}

QVideoFrameFormat QFFmpegScreenCaptureDxgi::frameFormat() const
{
    if (m_grabber)
        return m_grabber->format();
    else
        return {};
}

bool QFFmpegScreenCaptureDxgi::setActiveInternal(bool active)
{
    if (bool(m_grabber) == active)
        return true;

    if (m_grabber) {
        m_grabber.reset();
        return true;
    } else {
        QScreen *screen = this->screen() ? this->screen() : QGuiApplication::primaryScreen();
        auto maybeDxgiScreen = findDxgiScreen(screen);
        if (!maybeDxgiScreen) {
            qCDebug(qLcScreenCaptureDxgi) << maybeDxgiScreen.error();
            updateError(QScreenCapture::NotFound, maybeDxgiScreen.error());
            return false;
        }

        auto maybeDev = createD3D11Device(maybeDxgiScreen.value().adapter.Get());
        if (!maybeDev) {
            qCDebug(qLcScreenCaptureDxgi) << maybeDev.error();
            updateError(QScreenCapture::InternalError, maybeDev.error());
            return false;
        }

        auto maybeDupOutput = duplicateOutput(maybeDev.value().Get(), maybeDxgiScreen.value().output.Get());
        if (!maybeDupOutput) {
            qCDebug(qLcScreenCaptureDxgi) << maybeDupOutput.error();
            updateError(QScreenCapture::InternalError, maybeDupOutput.error());
            return false;
        }

        m_grabber.reset(new Grabber(*this, screen, maybeDev.value(), maybeDupOutput.value()));
        m_grabber->start();
        return true;
    }
}

QT_END_NAMESPACE
