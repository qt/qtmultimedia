// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qffmpegscreencapture_uwp_p.h"
#include <qthread.h>
#include <private/qabstractvideobuffer_p.h>

#include <unknwn.h>
#include <winrt/base.h>
#include <QtCore/private/qfactorycacheregistration_p.h>
// Workaround for Windows SDK bug.
// See https://github.com/microsoft/Windows.UI.Composition-Win32-Samples/issues/47
namespace winrt::impl
{
template <typename Async>
auto wait_for(Async const& async, Windows::Foundation::TimeSpan const& timeout);
}
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Graphics.Capture.h>
#include <winrt/Windows.Graphics.DirectX.h>
#include <winrt/Windows.Graphics.DirectX.Direct3D11.h>
#include <Windows.Graphics.Capture.h>
#include <Windows.Graphics.Capture.Interop.h>
#include <windows.graphics.directx.direct3d11.interop.h>

#include <D3d11.h>
#include <dxgi1_2.h>
#include <dwmapi.h>
#include <lowlevelmonitorconfigurationapi.h>
#include <physicalmonitorenumerationapi.h>

#include "qvideoframe.h"
#include <qtimer.h>
#include <qwindow.h>
#include <qloggingcategory.h>
#include <qguiapplication.h>
#include <private/qmultimediautils_p.h>
#include <private/qwindowsmultimediautils_p.h>
#include <qpa/qplatformscreen_p.h>

#include <memory>
#include <system_error>
#include <variant>

static Q_LOGGING_CATEGORY(qLcScreenCaptureUwp, "qt.multimedia.ffmpeg.screencapture.uwp")

namespace winrt {
    using namespace winrt::Windows::Graphics::Capture;
    using namespace winrt::Windows::Graphics::DirectX;
    using namespace winrt::Windows::Graphics::DirectX::Direct3D11;
}

QT_BEGIN_NAMESPACE

using namespace Windows::Graphics::DirectX::Direct3D11;
using namespace std::chrono;
using namespace QWindowsMultimediaUtils;

struct DeviceFramePool
{
    winrt::IDirect3DDevice iDirect3DDevice;
    winrt::com_ptr<ID3D11Device> d3d11dev;
    winrt::Direct3D11CaptureFramePool framePool;
};

class QUwpTextureVideoBuffer : public QAbstractVideoBuffer
{
public:
    QUwpTextureVideoBuffer(winrt::com_ptr<IDXGISurface> &&surface)
        : QAbstractVideoBuffer(QVideoFrame::NoHandle)
        , m_surface(surface)
    {
    }
    ~QUwpTextureVideoBuffer()
    {
        QUwpTextureVideoBuffer::unmap();
    }

    QVideoFrame::MapMode mapMode() const override { return m_mapMode; }

    MapData map(QVideoFrame::MapMode mode) override
    {
        if (m_mapMode != QVideoFrame::NotMapped)
            return {};

        if (mode == QVideoFrame::ReadOnly) {
            DXGI_MAPPED_RECT rect = {};
            HRESULT hr = m_surface->Map(&rect, DXGI_MAP_READ);
            if (SUCCEEDED(hr)) {
                DXGI_SURFACE_DESC desc = {};
                hr = m_surface->GetDesc(&desc);

                MapData md = {};
                md.nPlanes = 1;
                md.bytesPerLine[0] = rect.Pitch;
                md.data[0] = rect.pBits;
                md.size[0] = desc.Width * desc.Height;

                m_mapMode = QVideoFrame::ReadOnly;

                return md;
            } else {
                qCDebug(qLcScreenCaptureUwp) << "Failed to map DXGI surface" << errorString(hr);
                return {};
            }
        }

        return {};
    }

    void unmap() override
    {
        if (m_mapMode == QVideoFrame::NotMapped)
            return;

        HRESULT hr = m_surface->Unmap();
        if (FAILED(hr)) {
            qCDebug(qLcScreenCaptureUwp) << "Failed to unmap surface" << errorString(hr);
        }
        m_mapMode = QVideoFrame::NotMapped;
    }

private:
    QVideoFrame::MapMode m_mapMode = QVideoFrame::NotMapped;
    winrt::com_ptr<IDXGISurface> m_surface;
};

class ScreenGrabberActiveUwp : public QThread
{
    Q_OBJECT
public:
    ScreenGrabberActiveUwp(QFFmpegScreenCaptureUwp &screenCapture, DeviceFramePool &devicePool,
                           winrt::GraphicsCaptureItem item, qreal maxFrameRate)
        : m_screenCapture(screenCapture)
        , m_devicePool(devicePool)
        , m_session(devicePool.framePool.CreateCaptureSession(item))
        , m_frameSize(item.Size())
        , m_maxFrameRate(maxFrameRate)
    {
        connect(this, &ScreenGrabberActiveUwp::error, &screenCapture, &QPlatformScreenCapture::updateError);
    }

    void run() override
    {
        m_session.IsCursorCaptureEnabled(false);
        m_session.StartCapture();
        m_lastFrameTime = steady_clock::now();

        QTimer timer;
        const microseconds interval(quint64(1000000. / m_maxFrameRate));

        timer.setInterval(duration_cast<milliseconds>(interval));
        timer.setTimerType(Qt::PreciseTimer);
        timer.callOnTimeout(this, &ScreenGrabberActiveUwp::grabFrame, Qt::DirectConnection);
        timer.start();

        ScreenGrabberActiveUwp::grabFrame();

        exec();

        m_session.Close();
    }

    void grabFrame()
    {
        auto d3dFrame = m_devicePool.framePool.TryGetNextFrame();
        if (!d3dFrame)
            return;

        if (m_frameSize != d3dFrame.ContentSize()) {
            m_frameSize = d3dFrame.ContentSize();
            m_devicePool.framePool.Recreate(m_devicePool.iDirect3DDevice,
                                            winrt::DirectXPixelFormat::R8G8B8A8UIntNormalized, 1,
                                            d3dFrame.ContentSize());
        }

        auto d3dSurface = d3dFrame.Surface();
        winrt::com_ptr<IDirect3DDxgiInterfaceAccess> dxgiInterfaceAccess{ d3dSurface.as<IDirect3DDxgiInterfaceAccess>() };
        if (!dxgiInterfaceAccess)
            return;

        winrt::com_ptr<IDXGISurface> dxgiSurface;
        HRESULT hr = dxgiInterfaceAccess->GetInterface(__uuidof(dxgiSurface), dxgiSurface.put_void());
        if (FAILED(hr)) {
            emitError(QScreenCapture::CaptureFailed, "Failed to get DXGI surface interface", hr);
            return;
        }

        DXGI_SURFACE_DESC desc = {};
        hr = dxgiSurface->GetDesc(&desc);
        if (FAILED(hr)) {
            emitError(QScreenCapture::CaptureFailed, "Failed to get DXGI surface description", hr);
            return;
        }

        D3D11_TEXTURE2D_DESC texDesc = {};
        texDesc.Width = desc.Width;
        texDesc.Height = desc.Height;
        texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        texDesc.Usage = D3D11_USAGE_STAGING;
        texDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        texDesc.MiscFlags = 0;
        texDesc.BindFlags = 0;
        texDesc.ArraySize = 1;
        texDesc.MipLevels = 1;
        texDesc.SampleDesc = { 1, 0 };

        winrt::com_ptr<ID3D11Texture2D> texture;
        hr = m_devicePool.d3d11dev->CreateTexture2D(&texDesc, nullptr, texture.put());
        if (FAILED(hr)) {
            emitError(QScreenCapture::CaptureFailed, "Failed to create ID3D11Texture2D", hr);
            return;
        }

        winrt::com_ptr<ID3D11DeviceContext> ctx;
        m_devicePool.d3d11dev->GetImmediateContext(ctx.put());
        ctx->CopyResource(texture.as<ID3D11Resource>().get(), dxgiSurface.as<ID3D11Resource>().get());

        QVideoFrameFormat format(QSize{ int(desc.Width), int(desc.Height) }, QVideoFrameFormat::Format_RGBX8888);
        format.setFrameRate(m_maxFrameRate);

        QVideoFrame frame(new QUwpTextureVideoBuffer(std::move(texture.as<IDXGISurface>())),format);
        frame.setStartTime(duration_cast<microseconds>(m_lastFrameTime.time_since_epoch()).count());
        m_lastFrameTime = steady_clock::now();
        frame.setEndTime(duration_cast<microseconds>(m_lastFrameTime.time_since_epoch()).count());

        emit m_screenCapture.newVideoFrame(frame);
    }

signals:
    void error(QScreenCapture::Error code, const QString &desc);

private:
    void emitError(QScreenCapture::Error code, const QString &desc, HRESULT hr)
    {
        QString text = desc + QWindowsMultimediaUtils::errorString(hr);
        qCDebug(qLcScreenCaptureUwp) << text;
        emit error(code, text);
    }

    QFFmpegScreenCaptureUwp &m_screenCapture;
    DeviceFramePool m_devicePool;
    winrt::GraphicsCaptureSession m_session;
    winrt::Windows::Graphics::SizeInt32 m_frameSize;
    time_point<steady_clock> m_lastFrameTime = {};
    qreal m_maxFrameRate;
};

QFFmpegScreenCaptureUwp::QFFmpegScreenCaptureUwp(QScreenCapture *screenCapture)
    : QFFmpegScreenCaptureBase(screenCapture)
{
    qCDebug(qLcScreenCaptureUwp) << "Creating UWP screen capture";
}

QFFmpegScreenCaptureUwp::~QFFmpegScreenCaptureUwp()
{
    resetGrabber();
}

static QMaybe<DeviceFramePool>
createCaptureFramePool(IDXGIAdapter1 *adapter, const winrt::GraphicsCaptureItem &item)
{
    winrt::com_ptr<ID3D11Device> d3d11dev;
    HRESULT hr =
            D3D11CreateDevice(adapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr, D3D11_CREATE_DEVICE_DEBUG,
                              nullptr, 0, D3D11_SDK_VERSION, d3d11dev.put(), nullptr, nullptr);
    if (FAILED(hr))
        return { "Failed to create ID3D11Device device" + errorString(hr) };

    winrt::com_ptr<IDXGIDevice> dxgiDevice;
    hr = d3d11dev->QueryInterface(__uuidof(IDXGIDevice), dxgiDevice.put_void());
    if (FAILED(hr))
        return { "Failed to obtain dxgi for D3D11Device face" };

    winrt::IDirect3DDevice iDirect3DDevice{};
    hr = CreateDirect3D11DeviceFromDXGIDevice(
            dxgiDevice.get(), reinterpret_cast<::IInspectable **>(winrt::put_abi(iDirect3DDevice)));
    if (FAILED(hr))
        return { "Failed to create IDirect3DDevice device" + errorString(hr) };

    auto pool = winrt::Direct3D11CaptureFramePool::Create(
        iDirect3DDevice, winrt::DirectXPixelFormat::R8G8B8A8UIntNormalized, 1, item.Size());
    if (pool)
        return DeviceFramePool{ iDirect3DDevice, d3d11dev, pool };
    else
        return { "Failed to create capture frame pool" };
}

struct Monitor {
    winrt::com_ptr<IDXGIAdapter1> adapter;
    HMONITOR handle = nullptr;
};

static QMaybe<Monitor> findScreen(const QScreen *screen)
{
    if (!screen)
        return { "Cannot find nullptr screen" };

    auto *winScreen = screen->nativeInterface<QNativeInterface::Private::QWindowsScreen>();
    HMONITOR handle = winScreen ? winScreen->handle() : nullptr;

    winrt::com_ptr<IDXGIFactory1> factory;
    HRESULT hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1), factory.put_void());
    if (FAILED(hr))
        return { "Failed to create IDXGIFactory" + errorString(hr) };

    winrt::com_ptr<IDXGIAdapter1> adapter;
    for (quint32 i = 0; SUCCEEDED(factory->EnumAdapters1(i, adapter.put())); i++, adapter = {}) {
        winrt::com_ptr<IDXGIOutput> output;
        for (quint32 j = 0; SUCCEEDED(adapter->EnumOutputs(j, output.put())); j++, output = {}) {
            DXGI_OUTPUT_DESC desc = {};
            output->GetDesc(&desc);
            qCDebug(qLcScreenCaptureUwp) << i << j << QString::fromWCharArray(desc.DeviceName);
            auto match = handle ? handle == desc.Monitor
                                : QString::fromWCharArray(desc.DeviceName) == screen->name();
            if (match)
                return Monitor { adapter, desc.Monitor };
        }
    }
    return { "Could not find screen adapter " + screen->name() };
}

static QMaybe<Monitor> findScreenForWindow(HWND wh)
{
    HMONITOR handle = MonitorFromWindow(wh, MONITOR_DEFAULTTONULL);
    if (!handle)
        return { "Cannot find window screen" };

    winrt::com_ptr<IDXGIFactory1> factory;
    HRESULT hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1), factory.put_void());
    if (FAILED(hr))
        return { "Failed to create IDXGIFactory" + errorString(hr) };

    winrt::com_ptr<IDXGIAdapter1> adapter;
    for (quint32 i = 0; SUCCEEDED(factory->EnumAdapters1(i, adapter.put())); i++, adapter = {}) {
        winrt::com_ptr<IDXGIOutput> output;
        for (quint32 j = 0; SUCCEEDED(adapter->EnumOutputs(j, output.put())); j++, output = {}) {
            DXGI_OUTPUT_DESC desc = {};
            output->GetDesc(&desc);
            qCDebug(qLcScreenCaptureUwp) << i << j << QString::fromWCharArray(desc.DeviceName);
            if (desc.Monitor == handle)
                return Monitor { adapter, handle };
        }
    }

    return { "Could not find window screen adapter" };
}

static QMaybe<winrt::GraphicsCaptureItem> createScreenCaptureItem(HMONITOR handle)
{
    auto factory = winrt::get_activation_factory<winrt::GraphicsCaptureItem>();
    auto interop = factory.as<IGraphicsCaptureItemInterop>();

    winrt::GraphicsCaptureItem item = {nullptr};
    HRESULT hr = interop->CreateForMonitor(handle, __uuidof(ABI::Windows::Graphics::Capture::IGraphicsCaptureItem), winrt::put_abi(item));
    if (FAILED(hr))
        return "Failed to create capture item for monitor" + errorString(hr);
    else
        return item;
}

static QMaybe<winrt::GraphicsCaptureItem> createWindowCaptureItem(HWND handle)
{
    auto factory = winrt::get_activation_factory<winrt::GraphicsCaptureItem>();
    auto interop = factory.as<IGraphicsCaptureItemInterop>();

    winrt::GraphicsCaptureItem item = {nullptr};
    HRESULT hr = interop->CreateForWindow(handle, __uuidof(ABI::Windows::Graphics::Capture::IGraphicsCaptureItem), winrt::put_abi(item));
    if (FAILED(hr))
        return "Failed to create capture item for window" + errorString(hr);
    else
        return item;
}

static QString isCapturableWindow(HWND hwnd)
{
    if (hwnd == GetShellWindow())
        return "Cannot capture the shell window";

    wchar_t className[MAX_PATH] = {};
    GetClassName(hwnd, className, MAX_PATH);
    if (QString::fromWCharArray(className).length() == 0)
        return "Cannot capture windows without a class name";

    if (!IsWindowVisible(hwnd))
        return "Cannot capture invisible windows";

    if (GetParent(hwnd) != 0)
        return "Can only capture root windows";

    LONG style = GetWindowLong(hwnd, GWL_STYLE);
    if (style & WS_DISABLED)
        return "Cannot capture disabled windows";

    LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
    if (exStyle & WS_EX_TOOLWINDOW)
        return "No tooltips";

    DWORD cloaked = FALSE;
    HRESULT hr = DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &cloaked, sizeof(cloaked));
    if (SUCCEEDED(hr) && cloaked == DWM_CLOAKED_SHELL)
        return "Cannot capture cloaked windows";

    return {};
}

static qreal getMonitorRefreshRateHz(HMONITOR handle)
{
    DWORD count = 0;
    if (GetNumberOfPhysicalMonitorsFromHMONITOR(handle, &count)) {
        std::vector<PHYSICAL_MONITOR> monitors{ count };
        if (GetPhysicalMonitorsFromHMONITOR(handle, count, monitors.data())) {
            for (auto monitor : monitors) {
                MC_TIMING_REPORT screenTiming = {};
                if (GetTimingReport(monitor.hPhysicalMonitor, &screenTiming))
                    return qreal(screenTiming.dwVerticalFrequencyInHZ) / 100.;
            }
        }
    }
    return 60.;
}

bool QFFmpegScreenCaptureUwp::setActiveInternal(bool active)
{
    if (bool(m_screenGrabber) == active)
        return false;

    if (m_screenGrabber) {
        resetGrabber();
        m_format = {};
        return true;
    }

    QScreen *screen = this->screen();
    QWindow *window = this->window();
    WId wid = windowId();
    if (!screen && !wid && !window)
        screen = QGuiApplication::primaryScreen();

    auto windowHandle = window ? HWND(window->winId()) : HWND(wid);
    if (windowHandle) {
        QString error = isCapturableWindow(windowHandle);
        if (!error.isEmpty()) {
            emitError(QScreenCapture::InternalError, error);
            return false;
        }
    }

    auto maybeMonitor = screen ? findScreen(screen)
                               : findScreenForWindow(windowHandle);
    if (!maybeMonitor) {
        emitError(QScreenCapture::NotFound, maybeMonitor.error());
        return false;
    }

    auto maybeItem = screen ? createScreenCaptureItem(maybeMonitor.value().handle)
                            : createWindowCaptureItem(windowHandle);
    if (!maybeItem) {
        emitError(QScreenCapture::NotFound, maybeItem.error());
        return false;
    }

    auto maybePool = createCaptureFramePool(maybeMonitor.value().adapter.get(), maybeItem.value());
    if (!maybePool) {
        emitError(QScreenCapture::InternalError, maybePool.error());
        return false;
    }

    qreal refreshRate = getMonitorRefreshRateHz(maybeMonitor.value().handle);

    m_format = QVideoFrameFormat({ maybeItem.value().Size().Width, maybeItem.value().Size().Height },
                                 QVideoFrameFormat::Format_RGBX8888);
    m_format.setFrameRate(refreshRate);

    m_screenGrabber = std::make_unique<ScreenGrabberActiveUwp>(*this, maybePool.value(), maybeItem.value(), refreshRate);
    m_screenGrabber->start();

    return true;
}

bool QFFmpegScreenCaptureUwp::isSupported()
{
    return winrt::GraphicsCaptureSession::IsSupported();
}

QVideoFrameFormat QFFmpegScreenCaptureUwp::format() const
{
    return m_format;
}

void QFFmpegScreenCaptureUwp::emitError(QScreenCapture::Error code, const QString &desc)
{
    qCDebug(qLcScreenCaptureUwp) << desc;
    updateError(code, desc);
}

void QFFmpegScreenCaptureUwp::resetGrabber()
{
    if (m_screenGrabber) {
        m_screenGrabber->quit();
        m_screenGrabber->wait();
        m_screenGrabber.reset();
    }
}

QT_END_NAMESPACE

#include "qffmpegscreencapture_uwp.moc"
