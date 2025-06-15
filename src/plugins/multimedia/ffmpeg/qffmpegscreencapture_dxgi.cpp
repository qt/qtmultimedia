// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qffmpegscreencapture_dxgi_p.h"
#include "qffmpegsurfacecapturegrabber_p.h"
#include "qabstractvideobuffer.h"
#include <private/qmultimediautils_p.h>
#include <private/qvideoframe_p.h>
#include <QtGui/qscreen_platform.h>
#include "qvideoframe.h"

#include <qloggingcategory.h>
#include <qwaitcondition.h>
#include <qmutex.h>
#include <QtCore/private/qsystemerror_p.h>
#include <QtCore/private/qexpected_p.h>

#include "D3d11.h"
#include "dxgi1_2.h"

#include <system_error>

#include <mutex> // std::scoped_lock

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(qLcScreenCaptureDxgi, "qt.multimedia.ffmpeg.screencapturedxgi");

using namespace Qt::StringLiterals;

namespace {

// Convenience wrapper that combines an HRESULT
// status code with an optional textual description.
class ComStatus
{
public:
    ComStatus() = default;
    ComStatus(HRESULT hr) : m_hr{ hr } { }
    ComStatus(HRESULT hr, QAnyStringView msg) : m_hr{ hr }, m_msg{ msg.toString() } { }

    ComStatus(const ComStatus &) = default;
    ComStatus(ComStatus &&) = default;
    ComStatus &operator=(const ComStatus &) = default;
    ComStatus &operator=(ComStatus &&) = default;

    explicit operator bool() const { return m_hr == S_OK; }

    HRESULT code() const { return m_hr; }
    QString str() const
    {
        if (!m_msg)
            return QSystemError::windowsComString(m_hr);
        return *m_msg + u" " + QSystemError::windowsComString(m_hr);
    }

private:
    HRESULT m_hr = S_OK;
    std::optional<QString> m_msg;
};

template <typename T>
using ComProduct = q23::expected<ComPtr<T>, ComStatus>;
}

class QD3D11TextureVideoBuffer : public QAbstractVideoBuffer
{
public:
    QD3D11TextureVideoBuffer(const ComPtr<ID3D11Device> &device, std::shared_ptr<QMutex> &mutex,
                             const ComPtr<ID3D11Texture2D> &texture)
        : m_device(device), m_texture(texture), m_ctxMutex(mutex)
    {}

    ~QD3D11TextureVideoBuffer() { Q_ASSERT(m_mapMode == QVideoFrame::NotMapped); }

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
            mapData.planeCount = 1;
            mapData.bytesPerLine[0] = int(resource.RowPitch);
            mapData.data[0] = reinterpret_cast<uchar*>(resource.pData);
            mapData.dataSize[0] = int(texDesc.Height * resource.RowPitch);
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

    QVideoFrameFormat format() const override { return {}; }

    QSize getSize() const
    {
        if (!m_texture)
            return {};

        D3D11_TEXTURE2D_DESC desc{};
        m_texture->GetDesc(&desc);

        return { static_cast<int>(desc.Width), static_cast<int>(desc.Height) };
    }

private:
    ComPtr<ID3D11Device> m_device;
    ComPtr<ID3D11Texture2D> m_texture;
    ComPtr<ID3D11Texture2D> m_cpuTexture;
    ComPtr<ID3D11DeviceContext> m_ctx;
    std::shared_ptr<QMutex> m_ctxMutex;
    QVideoFrame::MapMode m_mapMode = QVideoFrame::NotMapped;
};

namespace {

struct DxgiScreen
{
    q23::expected<QSize, ComStatus> physicalSize() const
    {
        DXGI_OUTPUT_DESC desc{};
        const HRESULT hr = output->GetDesc(&desc);
        if (hr != S_OK)
            return q23::unexpected{ hr };

        const RECT bounds = desc.DesktopCoordinates;
        const QRect displayRect{ bounds.left, bounds.top,
                                 bounds.right - bounds.left,
                                 bounds.bottom - bounds.top };
        return displayRect.size();
    }

    q23::expected<QtVideo::Rotation, ComStatus> rotation() const
    {
        DXGI_OUTPUT_DESC desc{};
        const HRESULT hr = output->GetDesc(&desc);
        if (hr != S_OK)
            return q23::unexpected{ hr };

        switch (desc.Rotation) {

        case DXGI_MODE_ROTATION_ROTATE90:
            return QtVideo::Rotation::Clockwise90;
        case DXGI_MODE_ROTATION_ROTATE180:
            return QtVideo::Rotation::Clockwise180;
        case DXGI_MODE_ROTATION_ROTATE270:
            return QtVideo::Rotation::Clockwise270;
        default:
            return QtVideo::Rotation::None;
        }
    }

    ComPtr<IDXGIAdapter1> adapter;
    ComPtr<IDXGIOutput> output;
};

q23::expected<DxgiScreen, ComStatus> findDxgiScreen(const QScreen *screen)
{
    if (!screen)
        return q23::unexpected{ ComStatus{ E_FAIL, "Cannot find nullptr screen"_L1 } };

    auto *winScreen = screen->nativeInterface<QNativeInterface::QWindowsScreen>();
    HMONITOR handle = winScreen ? winScreen->handle() : nullptr;

    ComPtr<IDXGIFactory1> factory;
    HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&factory));
    if (FAILED(hr))
        return q23::unexpected{ ComStatus{ hr, "Failed to create IDXGIFactory"_L1 } };

    ComPtr<IDXGIAdapter1> adapter;
    for (quint32 i = 0; factory->EnumAdapters1(i, adapter.ReleaseAndGetAddressOf()) == S_OK; i++) {
        ComPtr<IDXGIOutput> output;
        for (quint32 j = 0; adapter->EnumOutputs(j, output.ReleaseAndGetAddressOf()) == S_OK; ++j) {
            DXGI_OUTPUT_DESC desc = {};
            output->GetDesc(&desc);
            qCDebug(qLcScreenCaptureDxgi) << i << j << QString::fromWCharArray(desc.DeviceName);
            auto match = handle ? handle == desc.Monitor
                                : QString::fromWCharArray(desc.DeviceName) == screen->name();
            if (match)
                return DxgiScreen{ adapter, output };
        }
    }
    return q23::unexpected{ ComStatus{
            DXGI_ERROR_NOT_FOUND,
            "Could not find screen adapter "_L1 + screen->name(),
    } };
}

class DxgiDuplication
{
public:
    ~DxgiDuplication()
    {
        if (m_releaseFrame)
            m_dup->ReleaseFrame();
    }

    ComStatus initialize(QScreen const *screen)
    {
        q23::expected<DxgiScreen, ComStatus> dxgiScreen = findDxgiScreen(screen);
        if (!dxgiScreen)
            return dxgiScreen.error();

        const ComPtr<IDXGIAdapter1> adapter = dxgiScreen->adapter;

        ComPtr<ID3D11Device> d3d11dev;
        HRESULT hr =
                D3D11CreateDevice(adapter.Get(), D3D_DRIVER_TYPE_UNKNOWN, nullptr, 0, nullptr, 0,
                                  D3D11_SDK_VERSION, d3d11dev.GetAddressOf(), nullptr, nullptr);
        if (FAILED(hr))
            return { hr, "Failed to create ID3D11Device device"_L1 };

        ComPtr<IDXGIOutput1> output;
        hr = dxgiScreen->output.As(&output);
        if (FAILED(hr))
            return { hr, "Failed to create IDXGIOutput1"_L1 };

        ComPtr<IDXGIOutputDuplication> dup;
        hr = output->DuplicateOutput(d3d11dev.Get(), dup.GetAddressOf());
        if (FAILED(hr))
            return { hr, "Failed to duplicate IDXGIOutput1"_L1 };

        m_adapter = dxgiScreen->adapter;
        m_output = output;
        m_device = d3d11dev;
        m_dup = dup;
        return { S_OK };
    }

    bool valid() const { return m_dup != nullptr; }
    q23::expected<std::unique_ptr<QD3D11TextureVideoBuffer>, ComStatus> getNextVideoFrame()
    {
        const ComProduct<ID3D11Texture2D> texture = getNextFrame();

        if (!texture)
            return q23::unexpected{ texture.error() };

        return std::make_unique<QD3D11TextureVideoBuffer>(m_device, m_ctxMutex, *texture);
    }

private:
    ComProduct<ID3D11Texture2D> getNextFrame()
    {
        std::scoped_lock guard{ *m_ctxMutex };

        if (m_releaseFrame) {
            m_releaseFrame = false;

            HRESULT hr = m_dup->ReleaseFrame();

            if (hr != S_OK)
                return q23::unexpected{ ComStatus{ hr,
                                                   "Failed to release duplication frame."_L1 } };
        }

        ComPtr<IDXGIResource> frame;
        DXGI_OUTDUPL_FRAME_INFO info;

        HRESULT hr = m_dup->AcquireNextFrame(0, &info, frame.GetAddressOf());

        if (hr != S_OK)
            return q23::unexpected{ ComStatus{ hr, "Failed to grab the screen content"_L1 } };

        m_releaseFrame = true;

        ComPtr<ID3D11Texture2D> tex;
        hr = frame.As(&tex);
        if (hr != S_OK)
            return q23::unexpected{ ComStatus{ hr, "Failed to obtain D3D11 texture"_L1 } };

        D3D11_TEXTURE2D_DESC texDesc = {};
        tex->GetDesc(&texDesc);
        texDesc.MiscFlags = 0;
        texDesc.BindFlags = 0;

        ComPtr<ID3D11Texture2D> texCopy;
        hr = m_device->CreateTexture2D(&texDesc, nullptr, texCopy.GetAddressOf());
        if (hr != S_OK)
            return q23::unexpected{ ComStatus{ hr,
                                               "Failed to create texture with CPU access"_L1 } };

        ComPtr<ID3D11DeviceContext> ctx;
        m_device->GetImmediateContext(ctx.GetAddressOf());
        ctx->CopyResource(texCopy.Get(), tex.Get());

        return texCopy;
    }

    ComPtr<IDXGIAdapter1> m_adapter;
    ComPtr<IDXGIOutput> m_output;
    ComPtr<ID3D11Device> m_device;
    ComPtr<IDXGIOutputDuplication> m_dup;
    bool m_releaseFrame = false;
    std::shared_ptr<QMutex> m_ctxMutex = std::make_shared<QMutex>();
};

q23::expected<QVideoFrameFormat, ComStatus> getFrameFormat(const QScreen *screen)
{
    const auto dxgiScreen = findDxgiScreen(screen);
    if (!dxgiScreen)
        return q23::unexpected{ dxgiScreen.error() };

    const auto screenSize = dxgiScreen->physicalSize();
    if (!screenSize)
        return q23::unexpected{ screenSize.error() };

    const auto rotation = dxgiScreen->rotation();
    if (!rotation)
        return q23::unexpected{ rotation.error() };

    QVideoFrameFormat format = { *screenSize, QVideoFrameFormat::Format_BGRA8888 };
    format.setRotation(*rotation);
    format.setStreamFrameRate(static_cast<int>(screen->refreshRate()));

    return format;
}

} // namespace

class QFFmpegScreenCaptureDxgi::Grabber : public QFFmpegSurfaceCaptureGrabber
{
public:
    Grabber(QFFmpegScreenCaptureDxgi &screenCapture, QScreen *screen,
            const QVideoFrameFormat &format)
        : m_screen(screen)
        , m_format(format)
    {
        setFrameRate(screen->refreshRate());
        addFrameCallback(screenCapture, &QFFmpegScreenCaptureDxgi::newVideoFrame);
        connect(this, &Grabber::errorUpdated, &screenCapture, &QFFmpegScreenCaptureDxgi::updateError);
    }

    ~Grabber() {
        stop();
    }

    QVideoFrameFormat format() {
        return m_format;
    }

    QVideoFrame grabFrame() override
    {
        QVideoFrame frame;
        if (!m_duplication.valid()) {
            const ComStatus status = m_duplication.initialize(m_screen);
            if (!status) {
                if (status.code() == E_ACCESSDENIED) {
                    // May occur for some time after pushing Ctrl+Alt+Del.
                    updateError(QPlatformSurfaceCapture::NoError, status.str());
                    qCWarning(qLcScreenCaptureDxgi) << status.str();
                }
                return frame;
            }
        }

        auto maybeBuf = m_duplication.getNextVideoFrame();
        if (maybeBuf) {
            std::unique_ptr<QD3D11TextureVideoBuffer> buffer = std::move(*maybeBuf);

            const QSize bufSize = buffer->getSize();
            if (bufSize != m_format.frameSize())
                m_format.setFrameSize(bufSize);

            frame = QVideoFramePrivate::createFrame(std::move(buffer), format());
        } else {
            const ComStatus &status = maybeBuf.error();

            if (status.code() == DXGI_ERROR_WAIT_TIMEOUT) {
                // All is good, we just didn't get a new frame yet
                updateError(QPlatformSurfaceCapture::NoError, status.str());
            } else if (status.code() == DXGI_ERROR_ACCESS_LOST) {
                // Can happen for example when pushing Ctrl + Alt + Del
                m_duplication = {};
                updateError(QPlatformSurfaceCapture::NoError, status.str());
                qCWarning(qLcScreenCaptureDxgi) << status.str();
            } else if (!status) {
                updateError(QPlatformSurfaceCapture::CaptureFailed, status.str());
                qCWarning(qLcScreenCaptureDxgi) << status.str();
            }
        }

        return frame;
    }

  protected:
    void initializeGrabbingContext() override
    {
        m_duplication = DxgiDuplication();
        const ComStatus status = m_duplication.initialize(m_screen);
        if (!status) {
            updateError(CaptureFailed, status.str());
            return;
        }

        QFFmpegSurfaceCaptureGrabber::initializeGrabbingContext();
    }

private:
    const QScreen *m_screen = nullptr;
    QVideoFrameFormat m_format;
    DxgiDuplication m_duplication;
};

QFFmpegScreenCaptureDxgi::QFFmpegScreenCaptureDxgi() : QPlatformSurfaceCapture(ScreenSource{}) { }

QFFmpegScreenCaptureDxgi::~QFFmpegScreenCaptureDxgi() = default;

QVideoFrameFormat QFFmpegScreenCaptureDxgi::frameFormat() const
{
    if (m_grabber)
        return m_grabber->format();
    return {};
}

bool QFFmpegScreenCaptureDxgi::setActiveInternal(bool active)
{
    if (static_cast<bool>(m_grabber) == active)
        return true;

    if (m_grabber) {
        m_grabber.reset();
    } else {
        auto screen = source<ScreenSource>();

        if (!checkScreenWithError(screen))
            return false;

        const auto format = getFrameFormat(screen);
        if (!format) {
            updateError(NotFound, "Unable to determine screen size or format"_L1 + format.error().str());
            return false;
        }

        m_grabber.reset(new Grabber(*this, screen, *format));
        m_grabber->start();
    }

    return true;
}

QT_END_NAMESPACE
