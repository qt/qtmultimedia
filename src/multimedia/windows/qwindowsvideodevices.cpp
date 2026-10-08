// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwindowsvideodevices_p.h"

#include <QtCore/quuid.h>
#include <QtCore/private/qcomptr_p.h>
#include <QtMultimedia/private/qcameradevice_p.h>
#include <QtMultimedia/private/qcomtaskresource_p.h>
#include <QtMultimedia/private/qwindowsmultimediautils_p.h>

#include <dbt.h>

#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>
#include <ks.h>

// MinGW-13.1 workaround
#ifndef KSCATEGORY_SENSOR_CAMERA
static constexpr GUID KSCATEGORY_SENSOR_CAMERA = {
    0x24e552d7, 0x6523, 0x47f7, { 0xa6, 0x47, 0xd3, 0x46, 0x5b, 0xf1, 0xf5, 0xca }
};
#endif

QT_BEGIN_NAMESPACE

namespace {

LRESULT QT_WIN_CALLBACK deviceNotificationWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_DEVICECHANGE) {
        auto b = (PDEV_BROADCAST_HDR)lParam;
        if (b && b->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE) {
            auto wmd = reinterpret_cast<QWindowsVideoDevices *>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
            if (wmd) {
                if (wParam == DBT_DEVICEARRIVAL || wParam == DBT_DEVICEREMOVECOMPLETE) {
                    wmd->onVideoInputsChanged();
                }
            }
        }
    }

    return 1;
}

LPCWSTR getWindowsClassName()
{
    // each Qt instance should have its own window class name to prevent name clashes when multiple
    // qt instances are used in the same process.
    // appending a uuid is good enough to ensure that.
    static const std::wstring singleton =
            QString(QStringLiteral("QWindowsMediaDevicesMessageWindow_")
                    + QUuid::createUuid().toString())
                    .toStdWString();
    Q_ASSERT(singleton.size() < 256 && "The maximum length for lpszClassName is 256");
    return singleton.c_str();
}

HWND createMessageOnlyWindow()
{
    WNDCLASSEX wx = {};
    wx.cbSize = sizeof(WNDCLASSEX);
    wx.lpfnWndProc = deviceNotificationWndProc;
    wx.hInstance = GetModuleHandle(nullptr);
    wx.lpszClassName = getWindowsClassName();

    if (!RegisterClassEx(&wx))
        return nullptr;

    auto hwnd = CreateWindowEx(0, getWindowsClassName(), TEXT("Message"), 0, 0, 0, 0, 0,
                               HWND_MESSAGE, nullptr, nullptr, nullptr);
    if (!hwnd) {
        UnregisterClass(getWindowsClassName(), GetModuleHandle(nullptr));
        return nullptr;
    }

    return hwnd;
}

} // namespace

QWindowsVideoDevices::QWindowsVideoDevices(QPlatformMediaIntegration *integration)
    : QPlatformVideoDevices(integration)
{
    m_videoDeviceMsgWindow = createMessageOnlyWindow();
    if (m_videoDeviceMsgWindow) {
        SetWindowLongPtr(m_videoDeviceMsgWindow, GWLP_USERDATA, (LONG_PTR)this);

        DEV_BROADCAST_DEVICEINTERFACE di = {};
        di.dbcc_size = sizeof(di);
        di.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
        di.dbcc_classguid = KSCATEGORY_VIDEO_CAMERA;

        m_videoDeviceNotification =
                RegisterDeviceNotification(m_videoDeviceMsgWindow, &di, DEVICE_NOTIFY_WINDOW_HANDLE);
        if (!m_videoDeviceNotification) {
            DestroyWindow(m_videoDeviceMsgWindow);
            m_videoDeviceMsgWindow = nullptr;

            UnregisterClass(getWindowsClassName(), GetModuleHandle(nullptr));
        }
    }

    if (!m_videoDeviceNotification) {
        qWarning() << "Video device change notification disabled";
    }
}

QWindowsVideoDevices::~QWindowsVideoDevices()
{
    if (m_videoDeviceNotification) {
        UnregisterDeviceNotification(m_videoDeviceNotification);
    }

    if (m_videoDeviceMsgWindow) {
        DestroyWindow(m_videoDeviceMsgWindow);
        UnregisterClass(getWindowsClassName(), GetModuleHandle(nullptr));
    }
}

static std::optional<QCameraFormat> createCameraFormat(IMFMediaType *mediaFormat)
{
    GUID subtype = GUID_NULL;
    if (FAILED(mediaFormat->GetGUID(MF_MT_SUBTYPE, &subtype)))
        return {};

    auto pixelFormat = QWindowsMultimediaUtils::pixelFormatFromMediaSubtype(subtype);
    if (pixelFormat == QVideoFrameFormat::Format_Invalid)
        return {};

    UINT32 width = 0u;
    UINT32 height = 0u;
    if (FAILED(MFGetAttributeSize(mediaFormat, MF_MT_FRAME_SIZE, &width, &height)))
        return {};
    QSize resolution{ int(width), int(height) };

    UINT32 num = 0u;
    UINT32 den = 0u;
    float minFr = 0.f;
    float maxFr = 0.f;

    if (SUCCEEDED(MFGetAttributeRatio(mediaFormat, MF_MT_FRAME_RATE_RANGE_MIN, &num, &den)))
        minFr = float(num) / float(den);

    if (SUCCEEDED(MFGetAttributeRatio(mediaFormat, MF_MT_FRAME_RATE_RANGE_MAX, &num, &den)))
        maxFr = float(num) / float(den);

    auto *f = new QCameraFormatPrivate{ QSharedData(), pixelFormat, resolution, minFr, maxFr };
    return f->create();
}

static QString getString(IMFActivate *device, const IID &id)
{
    QComTaskResource<WCHAR> str;
    UINT32 length = 0;
    HRESULT hr = device->GetAllocatedString(id, str.address(), &length);
    if (SUCCEEDED(hr)) {
        return QString::fromWCharArray(str.get());
    } else {
        return {};
    }
}

static std::optional<QCameraDevice> createCameraDevice(const QWindowsMediaFoundation &wmf,
                                                       IMFActivate *device)
{
    auto info = std::make_unique<QCameraDevicePrivate>();
    info->description = getString(device, MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME);
    info->id = getString(device, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK).toUtf8();

    IMFMediaSource *source = NULL;
    HRESULT hr = device->ActivateObject(IID_PPV_ARGS(&source));
    if (FAILED(hr))
        return {};

    ComPtr<IMFSourceReader> reader;
    hr = wmf.mfCreateSourceReaderFromMediaSource(source, NULL, reader.GetAddressOf());
    if (FAILED(hr))
        return {};

    QList<QSize> photoResolutions;
    QList<QCameraFormat> videoFormats;
    for (DWORD i = 0;; ++i) {
        // Loop through the supported formats for the video device
        ComPtr<IMFMediaType> mediaFormat;
        hr = reader->GetNativeMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, i,
                                        mediaFormat.GetAddressOf());
        if (FAILED(hr))
            break;

        auto maybeCamera = createCameraFormat(mediaFormat.Get());
        if (maybeCamera) {
            videoFormats << *maybeCamera;
            photoResolutions << maybeCamera->resolution();
        }
    }

    info->videoFormats = videoFormats;
    info->photoResolutions = photoResolutions;
    return info.release()->create();
}

static QList<QCameraDevice> readCameraDevices(const QWindowsMediaFoundation &wmf,
                                              IMFAttributes *attr)
{
    QList<QCameraDevice> cameras;
    UINT32 count = 0;
    IMFActivate **devicesRaw = nullptr;
    HRESULT hr = wmf.mfEnumDeviceSources(attr, &devicesRaw, &count);
    if (SUCCEEDED(hr)) {
        QComTaskResource<IMFActivate *[], QComDeleter> devices(devicesRaw, count);

        for (UINT32 i = 0; i < count; i++) {
            IMFActivate *device = devices[i];
            if (device) {
                auto maybeCamera = createCameraDevice(wmf, device);
                if (maybeCamera)
                    cameras << *maybeCamera;
            }
        }
    }
    return cameras;
}

QList<QCameraDevice> QWindowsVideoDevices::findVideoInputs() const
{
    if (!m_wmf)
        return {};

    QList<QCameraDevice> cameras;

    ComPtr<IMFAttributes> attr;
    HRESULT hr = m_wmf->mfCreateAttributes(attr.GetAddressOf(), 2);
    if (FAILED(hr))
        return {};

    hr = attr->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
                       MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);
    if (SUCCEEDED(hr)) {
        cameras << readCameraDevices(*m_wmf, attr.Get());

        hr = attr->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_CATEGORY,
                           KSCATEGORY_SENSOR_CAMERA);
        if (SUCCEEDED(hr))
            cameras << readCameraDevices(*m_wmf, attr.Get());
    }

    return cameras;
}

QT_END_NAMESPACE
