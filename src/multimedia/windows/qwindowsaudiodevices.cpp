// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwindowsaudiodevices_p.h"

#include <QtCore/qdebug.h>
#include <QtCore/qmap.h>
#include <QtCore/private/qcomobject_p.h>
#include <QtCore/private/qsystemerror_p.h>

#include <QtMultimedia/qmediadevices.h>
#include <QtMultimedia/private/qcomtaskresource_p.h>
#include <QtMultimedia/private/qwindowsaudiodevice_p.h>
#include <QtMultimedia/private/qwindowsaudiosink_p.h>
#include <QtMultimedia/private/qwindowsaudiosource_p.h>
#include <QtMultimedia/private/qwindows_propertystore_p.h>

#include <audioclient.h>
#include <functiondiscoverykeys_devpkey.h>
#include <mmdeviceapi.h>

QT_BEGIN_NAMESPACE

class CMMNotificationClient : public QComObject<IMMNotificationClient>
{
    ComPtr<IMMDeviceEnumerator> m_enumerator;
    QWindowsAudioDevices *m_windowsMediaDevices;
    QMap<QString, DWORD> m_deviceState;

public:
    CMMNotificationClient(QWindowsAudioDevices *windowsMediaDevices,
                          ComPtr<IMMDeviceEnumerator> enumerator,
                          QMap<QString, DWORD> &&deviceState)
        : m_enumerator(enumerator),
          m_windowsMediaDevices(windowsMediaDevices),
          m_deviceState(deviceState)
    {}

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
            m_windowsMediaDevices->onAudioInputsChanged();
        } else if (flow == EDataFlow::eRender) {
            m_windowsMediaDevices->onAudioOutputsChanged();
        }
    }

    void emitAudioDevicesChanged(LPCWSTR deviceID)
    {
        ComPtr<IMMDevice> device;
        ComPtr<IMMEndpoint> endpoint;
        EDataFlow flow;

        if (SUCCEEDED(m_enumerator->GetDevice(deviceID, device.GetAddressOf()))
            && SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&endpoint)))
            && SUCCEEDED(endpoint->GetDataFlow(&flow)))
        {
            emitAudioDevicesChanged(flow);
        }
    }

private:
    // Destructor is not public. Caller should call Release.
    ~CMMNotificationClient() override = default;
};

QWindowsAudioDevices::QWindowsAudioDevices()
    : QPlatformAudioDevices()
{
    auto hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_INPROC_SERVER,
                               IID_PPV_ARGS(&m_deviceEnumerator));

    if (FAILED(hr)) {
        qWarning("Failed to instantiate IMMDeviceEnumerator (%s)."
                 "Audio device change notification will be disabled",
            qPrintable(QSystemError::windowsComString(hr)));
        return;
    }

    QMap<QString, DWORD> devState;
    ComPtr<IMMDeviceCollection> devColl;
    UINT count = 0;

    if (SUCCEEDED(m_deviceEnumerator->EnumAudioEndpoints(EDataFlow::eAll, DEVICE_STATEMASK_ALL, devColl.GetAddressOf()))
        && SUCCEEDED(devColl->GetCount(&count)))
    {
        for (UINT i = 0; i < count; i++) {
            ComPtr<IMMDevice> device;
            DWORD state = 0;
            QComTaskResource<WCHAR> id;

            if (SUCCEEDED(devColl->Item(i, device.GetAddressOf()))
                && SUCCEEDED(device->GetState(&state))
                && SUCCEEDED(device->GetId(id.address()))) {
                devState.insert(QString::fromWCharArray(id.get()), state);
            }
        }
    }


    m_notificationClient = makeComObject<CMMNotificationClient>(this, m_deviceEnumerator, std::move(devState));
    m_deviceEnumerator->RegisterEndpointNotificationCallback(m_notificationClient.Get());
}

QWindowsAudioDevices::~QWindowsAudioDevices()
{
    if (m_deviceEnumerator) {
        // Note: Calling UnregisterEndpointNotificationCallback after CoUninitialize
        // will abruptly terminate application, preventing remaining destructors from
        // being called (QTBUG-120198).
        m_deviceEnumerator->UnregisterEndpointNotificationCallback(m_notificationClient.Get());
    }

    m_deviceEnumerator.Reset();
    m_notificationClient.Reset();
}

static std::optional<QString> getDeviceId(const ComPtr<IMMDevice> &dev)
{
    Q_ASSERT(dev);
    QComTaskResource<WCHAR> id;
    HRESULT status = dev->GetId(id.address());
    if (FAILED(status)) {
        qWarning() << "IMMDevice::GetId failed" << QSystemError::windowsComString(status);
        return {};
    }
    return QString::fromWCharArray(id.get());
}

QList<QAudioDevice> QWindowsAudioDevices::availableDevices(QAudioDevice::Mode mode) const
{
    using QtMultimediaPrivate::PropertyStoreHelper;
    if (!m_deviceEnumerator)
        return {};

    const bool audioOut = mode == QAudioDevice::Output;
    const auto dataFlow = audioOut ? EDataFlow::eRender : EDataFlow::eCapture;

    const auto defaultAudioDeviceID = [&, this]() -> std::optional<QString> {
        ComPtr<IMMDevice> dev;
        if (SUCCEEDED(m_deviceEnumerator->GetDefaultAudioEndpoint(dataFlow, ERole::eMultimedia,
                                                                  dev.GetAddressOf())))
            return getDeviceId(dev);

        return std::nullopt;
    }();

    QList<QAudioDevice> devices;

    ComPtr<IMMDeviceCollection> allActiveDevices;
    HRESULT result = m_deviceEnumerator->EnumAudioEndpoints(dataFlow, DEVICE_STATE_ACTIVE,
                                                            allActiveDevices.GetAddressOf());

    if (FAILED(result)) {
        qWarning() << "IMMDeviceEnumerator::EnumAudioEndpoints failed"
                   << QSystemError::windowsComString(result);
        return devices;
    }

    UINT numberOfDevices;
    result = allActiveDevices->GetCount(&numberOfDevices);
    if (FAILED(result)) {
        qWarning() << "IMMDeviceCollection::GetCount failed"
                   << QSystemError::windowsComString(result);
        return devices;
    }

    for (UINT index = 0; index != numberOfDevices; ++index) {
        ComPtr<IMMDevice> device;
        result = allActiveDevices->Item(index, device.GetAddressOf());
        if (FAILED(result)) {
            qWarning() << "IMMDeviceCollection::Item" << QSystemError::windowsComString(result);
            continue;
        }

        std::optional<QString> deviceId = getDeviceId(device);
        if (!deviceId)
            continue;

        q23::expected<PropertyStoreHelper, QString> props = PropertyStoreHelper::open(device);
        if (!props) {
            qWarning() << "OpenPropertyStore failed" << props.error();
            continue;
        }

        std::optional<QString> friendlyName = props->getString(PKEY_Device_FriendlyName);
        if (!friendlyName) {
            qWarning() << "Cannot read property store";
            continue;
        }

        auto dev = std::make_unique<QWindowsAudioDevice>(deviceId->toUtf8(), device, *friendlyName,
                                                         mode);
        dev->isDefault = deviceId == defaultAudioDeviceID;
        devices.append(QAudioDevicePrivate::createQAudioDevice(std::move(dev)));
    }

    return devices;
}

QList<QAudioDevice> QWindowsAudioDevices::findAudioInputs() const
{
    return availableDevices(QAudioDevice::Input);
}

QList<QAudioDevice> QWindowsAudioDevices::findAudioOutputs() const
{
    return availableDevices(QAudioDevice::Output);
}

QPlatformAudioSource *QWindowsAudioDevices::createAudioSource(const QAudioDevice &device,
                                                              const QAudioFormat &fmt,
                                                              QObject *parent)
{
    return new QtWASAPI::QWindowsAudioSource(device, fmt, parent);
}

QPlatformAudioSink *QWindowsAudioDevices::createAudioSink(const QAudioDevice &device,
                                                          const QAudioFormat &fmt, QObject *parent)
{
    return new QtWASAPI::QWindowsAudioSink(device, fmt, parent);
}

QT_END_NAMESPACE
