/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "qwindowsmediadevices_p.h"
#include "qmediadevices.h"
#include "qcameradevice_p.h"
#include "qvarlengtharray.h"

#include "private/qwindowsaudiosource_p.h"
#include "private/qwindowsaudiosink_p.h"
#include "private/qwindowsaudiodevice_p.h"
#include "private/qwindowsmultimediautils_p.h"

#include <private/mftvideo_p.h>

#include <Dbt.h>
#include <ks.h>

#include <mmsystem.h>
#include <mmddk.h>
#include <mfapi.h>
#include <mfobjects.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <Mferror.h>
#include <mmdeviceapi.h>
#include <Functiondiscoverykeys_devpkey.h>
#include <private/qwindowsaudioutils_p.h>
#include <private/qwindowsmfdefs_p.h>

QT_BEGIN_NAMESPACE

class CMMNotificationClient : public IMMNotificationClient
{
    LONG m_cRef;
    QWindowsIUPointer<IMMDeviceEnumerator> m_enumerator;
    QWindowsMediaDevices *m_windowsMediaDevices;
    QMap<QString, DWORD> m_deviceState;

public:
    CMMNotificationClient(QWindowsMediaDevices *windowsMediaDevices,
                          QWindowsIUPointer<IMMDeviceEnumerator> enumerator,
                          QMap<QString, DWORD> &&deviceState) :
        m_cRef(1),
        m_enumerator(enumerator),
        m_windowsMediaDevices(windowsMediaDevices),
        m_deviceState(deviceState)
    {}

    virtual ~CMMNotificationClient() {}

    // IUnknown methods -- AddRef, Release, and QueryInterface
    ULONG STDMETHODCALLTYPE AddRef() override
    {
        return InterlockedIncrement(&m_cRef);
    }

    ULONG STDMETHODCALLTYPE Release() override
    {
        ULONG ulRef = InterlockedDecrement(&m_cRef);
        if (0 == ulRef) {
            delete this;
        }
        return ulRef;
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, VOID **ppvInterface) override
    {
        if (IID_IUnknown == riid) {
            AddRef();
            *ppvInterface = (IUnknown*)this;
        } else if (__uuidof(IMMNotificationClient) == riid) {
            AddRef();
            *ppvInterface = (IMMNotificationClient*)this;
        } else {
            *ppvInterface = NULL;
            return E_NOINTERFACE;
        }
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE OnDefaultDeviceChanged(EDataFlow flow, ERole role, LPCWSTR) override
    {
        if (role == ERole::eMultimedia)
            emitAudioDevicesChanged(flow);

        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE OnDeviceAdded(LPCWSTR deviceID) override
    {
        auto it = m_deviceState.find(QString::fromWCharArray(deviceID));
        if (it == std::end(m_deviceState)) {
            m_deviceState.insert(QString::fromWCharArray(deviceID), DEVICE_STATE_ACTIVE);
            emitAudioDevicesChanged(deviceID);
        }

        return S_OK;
    };

    HRESULT STDMETHODCALLTYPE OnDeviceRemoved(LPCWSTR deviceID) override
    {
        auto key = QString::fromWCharArray(deviceID);
        auto it = m_deviceState.find(key);
        if (it != std::end(m_deviceState)) {
            if (it.value() == DEVICE_STATE_ACTIVE)
                emitAudioDevicesChanged(deviceID);
            m_deviceState.remove(key);
        }

        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE OnDeviceStateChanged(LPCWSTR deviceID, DWORD newState) override
    {
        if (auto it = m_deviceState.find(QString::fromWCharArray(deviceID)); it != std::end(m_deviceState)) {
            // If either the old state or the new state is active emit device change
            if ((it.value() == DEVICE_STATE_ACTIVE) != (newState == DEVICE_STATE_ACTIVE)) {
                emitAudioDevicesChanged(deviceID);
            }
            it.value() = newState;
        }

        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE OnPropertyValueChanged(LPCWSTR, const PROPERTYKEY) override
    {
        return S_OK;
    }

    void emitAudioDevicesChanged(EDataFlow flow)
    {
        // windowsMediaDevice may be deleted as we are executing the callback
        if (flow == EDataFlow::eCapture) {
            m_windowsMediaDevices->audioInputsChanged();
        } else if (flow == EDataFlow::eRender) {
            m_windowsMediaDevices->audioOutputsChanged();
        }
    }

    void emitAudioDevicesChanged(LPCWSTR deviceID)
    {
        QWindowsIUPointer<IMMDevice> device;
        QWindowsIUPointer<IMMEndpoint> endpoint;
        EDataFlow flow;

        if (SUCCEEDED(m_enumerator->GetDevice(deviceID, device.address()))
            && SUCCEEDED(device->QueryInterface(__uuidof(IMMEndpoint), (void**)endpoint.address()))
            && SUCCEEDED(endpoint->GetDataFlow(&flow)))
        {
                    emitAudioDevicesChanged(flow);
        }
    }
};

LRESULT QT_WIN_CALLBACK deviceNotificationWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_DEVICECHANGE) {
        auto b = (PDEV_BROADCAST_HDR)lParam;
        if (b && b->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE) {
            auto wmd = reinterpret_cast<QWindowsMediaDevices *>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

            if (wmd) {
                if (wParam == DBT_DEVICEARRIVAL) {
                    wmd->videoInputsChanged();
                } else if (wParam == DBT_DEVICEREMOVECOMPLETE) {
                    wmd->videoInputsChanged();
                }
            }
        }
    }

    return 1;
}

static const auto windowClassName = TEXT("QWindowsMediaDevicesMessageWindow");

HWND createMessageOnlyWindow()
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

QWindowsMediaDevices::QWindowsMediaDevices()
    : QPlatformMediaDevices(),
      m_videoDeviceMsgWindow(nullptr),
      m_videoDeviceNotification(nullptr)

{
    CoInitialize(nullptr);

    auto hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
                CLSCTX_INPROC_SERVER,__uuidof(IMMDeviceEnumerator),
                (void**)&m_deviceEnumerator);

    if (SUCCEEDED(hr)) {
        QMap<QString, DWORD> devState;
        QWindowsIUPointer<IMMDeviceCollection> devColl;
        UINT count = 0;

        if (SUCCEEDED(m_deviceEnumerator->EnumAudioEndpoints(EDataFlow::eAll, DEVICE_STATEMASK_ALL, devColl.address()))
            && SUCCEEDED(devColl->GetCount(&count)))
        {
            for (UINT i = 0; i < count; i++) {
                QWindowsIUPointer<IMMDevice> device;
                DWORD state = 0;
                LPWSTR id = nullptr;

                if (SUCCEEDED(devColl->Item(i, device.address()))
                    && SUCCEEDED(device->GetState(&state))
                    && SUCCEEDED(device->GetId(&id)))
                {
                    devState.insert(QString::fromWCharArray(id), state);
                    CoTaskMemFree(id);
                }
            }
        }


        m_notificationClient.reset(new CMMNotificationClient(this, m_deviceEnumerator, std::move(devState)));
        m_deviceEnumerator->RegisterEndpointNotificationCallback(m_notificationClient.get());

    } else {
        qWarning() << "Audio device change notification disabled";
    }

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

QWindowsMediaDevices::~QWindowsMediaDevices()
{
    if (m_deviceEnumerator) {
        m_deviceEnumerator->UnregisterEndpointNotificationCallback(m_notificationClient.get());
    }

    m_deviceEnumerator.reset();
    m_notificationClient.reset();

    if (m_videoDeviceNotification) {
        UnregisterDeviceNotification(m_videoDeviceNotification);
    }

    if (m_videoDeviceMsgWindow) {
        DestroyWindow(m_videoDeviceMsgWindow);
        UnregisterClass(windowClassName, GetModuleHandle(nullptr));
    }

    CoUninitialize();
}

QList<QAudioDevice> QWindowsMediaDevices::availableDevices(QAudioDevice::Mode mode) const
{
    const auto audioOut = mode == QAudioDevice::Output;

    const auto defaultAudioDeviceID = [this, audioOut]{
        const auto dataFlow = audioOut ? EDataFlow::eRender : EDataFlow::eCapture;
        QWindowsIUPointer<IMMDevice> dev;
        LPWSTR id = nullptr;
        QString sid;

        if (SUCCEEDED(m_deviceEnumerator->GetDefaultAudioEndpoint(dataFlow, ERole::eMultimedia, dev.address()))
            && SUCCEEDED(dev->GetId(&id))) {
            sid = QString::fromWCharArray(id);
            CoTaskMemFree(id);
        }
        return sid.toUtf8();
    }();

    QList<QAudioDevice> devices;

    auto waveDevices = audioOut ? waveOutGetNumDevs() : waveInGetNumDevs();

    for (auto waveID = 0u; waveID < waveDevices; waveID++) {
        auto wave = IntToPtr(waveID);
        auto waveMessage = [wave, audioOut](UINT msg, auto p0, auto p1) {
            return audioOut ? waveOutMessage((HWAVEOUT)wave, msg, (DWORD_PTR)p0, (DWORD_PTR)p1)
                            : waveInMessage((HWAVEIN)wave, msg, (DWORD_PTR)p0, (DWORD_PTR)p1);
        };

        size_t len = 0;
        if (waveMessage(DRV_QUERYFUNCTIONINSTANCEIDSIZE, &len, 0) != MMSYSERR_NOERROR)
            continue;

        QVarLengthArray<WCHAR> id(len);
        if (waveMessage(DRV_QUERYFUNCTIONINSTANCEID, id.data(), len) != MMSYSERR_NOERROR)
            continue;

        QWindowsIUPointer<IMMDevice> device;
        QWindowsIUPointer<IPropertyStore> props;
        if (FAILED(m_deviceEnumerator->GetDevice(id.data(), device.address()))
            || FAILED(device->OpenPropertyStore(STGM_READ, props.address()))) {
            continue;
        }

        PROPVARIANT varName;
        PropVariantInit(&varName);

        if (SUCCEEDED(props->GetValue(QMM_PKEY_Device_FriendlyName, &varName))) {
            auto description = QString::fromWCharArray(varName.pwszVal);
            auto strID = QString::fromWCharArray(id.data()).toUtf8();

            auto dev = new QWindowsAudioDeviceInfo(strID, device, waveID, description, mode);
            dev->isDefault = strID == defaultAudioDeviceID;

            devices.append(dev->create());
        }
        PropVariantClear(&varName);
    }

    return devices;
}

QList<QAudioDevice> QWindowsMediaDevices::audioInputs() const
{
    return availableDevices(QAudioDevice::Input);
}

QList<QAudioDevice> QWindowsMediaDevices::audioOutputs() const
{
    return availableDevices(QAudioDevice::Output);
}

QList<QCameraDevice> QWindowsMediaDevices::videoInputs() const
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
                    hr = ppDevices[index]->ActivateObject(
                            IID_PPV_ARGS(&pSource));
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

QPlatformAudioSource *QWindowsMediaDevices::createAudioSource(const QAudioDevice &deviceInfo)
{
    const auto *devInfo = static_cast<const QWindowsAudioDeviceInfo *>(deviceInfo.handle());
    return new QWindowsAudioSource(devInfo->waveId());
}

QPlatformAudioSink *QWindowsMediaDevices::createAudioSink(const QAudioDevice &deviceInfo)
{
    const auto *devInfo = static_cast<const QWindowsAudioDeviceInfo *>(deviceInfo.handle());
    return new QWindowsAudioSink(devInfo->immDev());
}

QT_END_NAMESPACE
