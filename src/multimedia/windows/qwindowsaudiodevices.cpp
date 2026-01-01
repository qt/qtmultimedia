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

namespace QtWASAPI {

namespace {

enum class DeviceState : uint8_t {
    active,
    disabled,
    notPresent,
    unplugged,
};

constexpr DeviceState asDeviceState(DWORD state)
{
    switch (state) {
    case DEVICE_STATE_ACTIVE:
        return DeviceState::active;
    case DEVICE_STATE_DISABLED:
        return DeviceState::disabled;
    case DEVICE_STATE_NOTPRESENT:
        return DeviceState::notPresent;
    case DEVICE_STATE_UNPLUGGED:
        return DeviceState::unplugged;
    default:
        Q_UNREACHABLE_RETURN(DeviceState::notPresent);
    }
}

} // namespace

class CMMNotificationClient : public QComObject<IMMNotificationClient>
{
    ComPtr<IMMDeviceEnumerator> m_enumerator;
    QWindowsAudioDevices *m_windowsMediaDevices;

    struct DeviceRecord
    {
        ComPtr<IMMDevice> device;
        DeviceState state;
    };

    QMap<QString, DeviceRecord> m_deviceMap;

public:
    CMMNotificationClient(QWindowsAudioDevices *windowsMediaDevices,
                          ComPtr<IMMDeviceEnumerator> enumerator)
        : m_enumerator(enumerator), m_windowsMediaDevices(windowsMediaDevices)
    {
        ComPtr<IMMDeviceCollection> devColl;
        UINT count = 0;

        if (SUCCEEDED(m_enumerator->EnumAudioEndpoints(EDataFlow::eAll, DEVICE_STATEMASK_ALL,
                                                       devColl.GetAddressOf()))
            && SUCCEEDED(devColl->GetCount(&count))) {
            for (UINT i = 0; i < count; i++) {
                ComPtr<IMMDevice> device;
                if (FAILED(devColl->Item(i, device.GetAddressOf())))
                    continue;

                auto enumerateResult = enumerateDevice(device);
                if (!enumerateResult)
                    continue;

                auto idResult = deviceId(enumerateResult->device);
                if (!idResult)
                    continue;

                m_deviceMap.insert(std::move(*idResult), std::move(*enumerateResult));
            }
        }
    }

private:
    HRESULT STDMETHODCALLTYPE OnDefaultDeviceChanged(EDataFlow flow, ERole role, LPCWSTR) override
    {
        if (role == ERole::eMultimedia)
            emitAudioDevicesChanged(flow);

        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE OnDeviceAdded(LPCWSTR deviceID) override
    {
        auto it = m_deviceMap.find(QString::fromWCharArray(deviceID));
        if (it == std::end(m_deviceMap)) {
            auto enumerateResult = enumerateDevice(deviceID);
            if (!enumerateResult)
                return S_OK;

            m_deviceMap.insert(QString::fromWCharArray(deviceID), *enumerateResult);

            if (enumerateResult->state == DeviceState::active)
                emitAudioDevicesChanged(deviceID);
        }

        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE OnDeviceRemoved(LPCWSTR deviceID) override
    {
        auto key = QString::fromWCharArray(deviceID);
        auto it = m_deviceMap.find(key);
        if (it != std::end(m_deviceMap)) {
            if (it.value().state == DeviceState::active)
                emitAudioDevicesChanged(deviceID);
            m_deviceMap.remove(key);
        }

        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE OnDeviceStateChanged(LPCWSTR deviceID, DWORD newState) override
    {
        if (auto it = m_deviceMap.find(QString::fromWCharArray(deviceID));
            it != std::end(m_deviceMap)) {
            // If either the old state or the new state is active emit device change
            auto oldAndNewState = QVector<DeviceState>{
                it.value().state,
                asDeviceState(newState),
            };
            if (oldAndNewState.contains(DeviceState::active))
                emitAudioDevicesChanged(deviceID);

            it.value().state = asDeviceState(newState);
        }

        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE OnPropertyValueChanged(LPCWSTR, const PROPERTYKEY) override
    {
        // TODO: re-enumerate
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

    q23::expected<DeviceRecord, HRESULT> enumerateDevice(LPCWSTR deviceID)
    {
        ComPtr<IMMDevice> device;
        auto deviceStatus = m_enumerator->GetDevice(deviceID, device.GetAddressOf());
        if (FAILED(deviceStatus))
            return q23::unexpected{ deviceStatus };
        return enumerateDevice(device);
    }

    q23::expected<DeviceRecord, HRESULT> enumerateDevice(const ComPtr<IMMDevice> &device)
    {
        DWORD state = 0;

        auto stateStatus = device->GetState(&state);
        if (FAILED(stateStatus))
            return q23::unexpected{ stateStatus };
        return DeviceRecord{
            device,
            asDeviceState(state),
        };
    }
    q23::expected<QString, HRESULT> deviceId(const ComPtr<IMMDevice> &device)
    {
        QComTaskResource<WCHAR> id;
        auto idStatus = device->GetId(id.address());
        if (FAILED(idStatus))
            return q23::unexpected{ idStatus };
        return QString::fromWCharArray(id.get());
    }

    // Destructor is not public. Caller should call Release.
    ~CMMNotificationClient() override = default;
};

} // namespace QtWASAPI

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

    m_notificationClient = makeComObject<QtWASAPI::CMMNotificationClient>(this, m_deviceEnumerator);
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
