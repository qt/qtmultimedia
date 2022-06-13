/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include "qwindowsvideodevices_p.h"

#include <private/qcameradevice_p.h>
#include <private/qwindowsmfdefs_p.h>
#include <private/qwindowsmultimediautils_p.h>

#include <Dbt.h>

#include <mfapi.h>
#include <mfreadwrite.h>
#include <Mferror.h>

QT_BEGIN_NAMESPACE

LRESULT QT_WIN_CALLBACK deviceNotificationWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_DEVICECHANGE) {
        auto b = (PDEV_BROADCAST_HDR)lParam;
        if (b && b->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE) {
            auto wmd = reinterpret_cast<QWindowsVideoDevices *>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
            if (wmd) {
                if (wParam == DBT_DEVICEARRIVAL || wParam == DBT_DEVICEREMOVECOMPLETE) {
                    wmd->videoInputsChanged();
                }
            }
        }
    }

    return 1;
}

static const auto windowClassName = TEXT("QWindowsMediaDevicesMessageWindow");

static HWND createMessageOnlyWindow()
{
    WNDCLASSEX wx = {};
    wx.cbSize = sizeof(WNDCLASSEX);
    wx.lpfnWndProc = deviceNotificationWndProc;
    wx.hInstance = GetModuleHandle(nullptr);
    wx.lpszClassName = windowClassName;

    if (!RegisterClassEx(&wx))
        return nullptr;

    auto hwnd = CreateWindowEx(0, windowClassName, TEXT("Message"),
                               0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, nullptr, nullptr);
    if (!hwnd) {
        UnregisterClass(windowClassName, GetModuleHandle(nullptr));
        return nullptr;
    }

    return hwnd;
}

QWindowsVideoDevices::QWindowsVideoDevices(QPlatformMediaIntegration *integration)
    : QPlatformVideoDevices(integration)
{
    m_videoDeviceMsgWindow = createMessageOnlyWindow();
    if (m_videoDeviceMsgWindow) {
        SetWindowLongPtr(m_videoDeviceMsgWindow, GWLP_USERDATA, (LONG_PTR)this);

        DEV_BROADCAST_DEVICEINTERFACE di = {};
        di.dbcc_size = sizeof(di);
        di.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
        di.dbcc_classguid = QMM_KSCATEGORY_VIDEO_CAMERA;

        m_videoDeviceNotification =
                RegisterDeviceNotification(m_videoDeviceMsgWindow, &di, DEVICE_NOTIFY_WINDOW_HANDLE);
        if (!m_videoDeviceNotification) {
            DestroyWindow(m_videoDeviceMsgWindow);
            m_videoDeviceMsgWindow = nullptr;

            UnregisterClass(windowClassName, GetModuleHandle(nullptr));
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
        UnregisterClass(windowClassName, GetModuleHandle(nullptr));
    }
}

QList<QCameraDevice> QWindowsVideoDevices::videoDevices() const
{
    QList<QCameraDevice> cameras;

    IMFAttributes *pAttributes = NULL;
    IMFActivate **ppDevices = NULL;

    // Create an attribute store to specify the enumeration parameters.
    HRESULT hr = MFCreateAttributes(&pAttributes, 1);
    if (SUCCEEDED(hr)) {
        // Source type: video capture devices
        hr = pAttributes->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
                                  MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);

        if (SUCCEEDED(hr)) {
            // Enumerate devices.
            UINT32 count;
            hr = MFEnumDeviceSources(pAttributes, &ppDevices, &count);
            if (SUCCEEDED(hr)) {
                // Iterate through devices.
                for (int index = 0; index < int(count); index++) {
                    QCameraDevicePrivate *info = new QCameraDevicePrivate;

                    IMFMediaSource *pSource = NULL;
                    IMFSourceReader *reader = NULL;

                    WCHAR *deviceName = NULL;
                    UINT32 deviceNameLength = 0;
                    UINT32 deviceIdLength = 0;
                    WCHAR *deviceId = NULL;

                    hr = ppDevices[index]->GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME,
                                                              &deviceName, &deviceNameLength);
                    if (SUCCEEDED(hr))
                        info->description = QString::fromWCharArray(deviceName);
                    CoTaskMemFree(deviceName);

                    hr = ppDevices[index]->GetAllocatedString(
                            MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK, &deviceId,
                            &deviceIdLength);
                    if (SUCCEEDED(hr))
                        info->id = QString::fromWCharArray(deviceId).toUtf8();
                    CoTaskMemFree(deviceId);

                    // Create the media source object.
                    ppDevices[index]->ActivateObject(IID_PPV_ARGS(&pSource));
                    // Create the media source reader.
                    hr = MFCreateSourceReaderFromMediaSource(pSource, NULL, &reader);
                    if (SUCCEEDED(hr)) {
                        QList<QSize> photoResolutions;
                        QList<QCameraFormat> videoFormats;

                        DWORD dwMediaTypeIndex = 0;
                        IMFMediaType *mediaFormat = NULL;
                        GUID subtype = GUID_NULL;
                        HRESULT mediaFormatResult = S_OK;

                        UINT32 frameRateMin = 0u;
                        UINT32 frameRateMax = 0u;
                        UINT32 denominator = 0u;
                        UINT32 width = 0u;
                        UINT32 height = 0u;

                        while (SUCCEEDED(mediaFormatResult)) {
                            // Loop through the supported formats for the video device
                            mediaFormatResult = reader->GetNativeMediaType(
                                    (DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, dwMediaTypeIndex,
                                    &mediaFormat);
                            if (mediaFormatResult == MF_E_NO_MORE_TYPES)
                                break;
                            else if (SUCCEEDED(mediaFormatResult)) {
                                QVideoFrameFormat::PixelFormat pixelFormat = QVideoFrameFormat::Format_Invalid;
                                QSize resolution;
                                float minFr = .0;
                                float maxFr = .0;

                                if (SUCCEEDED(mediaFormat->GetGUID(MF_MT_SUBTYPE, &subtype)))
                                    pixelFormat = QWindowsMultimediaUtils::pixelFormatFromMediaSubtype(subtype);

                                if (SUCCEEDED(MFGetAttributeSize(mediaFormat, MF_MT_FRAME_SIZE, &width,
                                                                 &height))) {
                                    resolution.rheight() = (int)height;
                                    resolution.rwidth() = (int)width;
                                    photoResolutions << resolution;
                                }

                                if (SUCCEEDED(MFGetAttributeRatio(mediaFormat, MF_MT_FRAME_RATE_RANGE_MIN,
                                                                  &frameRateMin, &denominator)))
                                    minFr = qreal(frameRateMin) / denominator;
                                if (SUCCEEDED(MFGetAttributeRatio(mediaFormat, MF_MT_FRAME_RATE_RANGE_MAX,
                                                                  &frameRateMax, &denominator)))
                                    maxFr = qreal(frameRateMax) / denominator;

                                auto *f = new QCameraFormatPrivate { QSharedData(), pixelFormat,
                                                                    resolution, minFr, maxFr };
                                videoFormats << f->create();
                            }
                            ++dwMediaTypeIndex;
                        }
                        if (mediaFormat)
                            mediaFormat->Release();

                        info->videoFormats = videoFormats;
                        info->photoResolutions = photoResolutions;
                    }
                    if (reader)
                        reader->Release();
                    cameras.append(info->create());
                }
            }
            for (DWORD i = 0; i < count; i++) {
                if (ppDevices[i])
                    ppDevices[i]->Release();
            }
            CoTaskMemFree(ppDevices);
        }
    }
    if (pAttributes)
        pAttributes->Release();

    return cameras;
}

QT_END_NAMESPACE
