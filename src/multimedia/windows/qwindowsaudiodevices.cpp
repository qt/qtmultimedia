// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwindowsaudiodevices_p.h"

#include <QtCore/qdebug.h>
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

#include <map>

QT_BEGIN_NAMESPACE

// older mingw does not have PKEY_Device_ContainerId defined
// https://github.com/mingw-w64/mingw-w64/commit/7e6eca69655c81976acfd7cd6a1ed25e7961e8c7
// defining it here to avoid depending on the mingw version
DEFINE_PROPERTYKEY(PKEY_Device_ContainerIdQt, 0x8c7ed206, 0x3f8a, 0x4827, 0xb3, 0xab, 0xae, 0x9e,
                   0x1f, 0xae, 0xfc, 0x6c, 2);

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

class CMMNotificationClient : public QObject, public QComObject<IMMNotificationClient>
{
    Q_OBJECT

    ComPtr<IMMDeviceEnumerator> m_enumerator;

    struct DeviceRecord
    {
        ComPtr<IMMDevice> device;
        DeviceState state;
    };

    std::map<QString, DeviceRecord> m_deviceMap;

public:
    explicit CMMNotificationClient(ComPtr<IMMDeviceEnumerator> enumerator)
        : m_enumerator(enumerator)
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

                m_deviceMap.emplace(std::move(*idResult), std::move(*enumerateResult));
            }
        }

        // Does not seem to be necessary, but also won't do any harm
        qRegisterMetaType<ComPtr<IMMDevice>>();
    }

signals:
    void audioDeviceAdded(ComPtr<IMMDevice>);
    void audioDeviceRemoved(ComPtr<IMMDevice>);
    void audioDevicePropertyChanged(ComPtr<IMMDevice>);
    void audioDeviceDefaultChanged(QAudioDevice::Mode, ComPtr<IMMDevice>);

private:
    HRESULT STDMETHODCALLTYPE OnDefaultDeviceChanged(EDataFlow flow, ERole role,
                                                     LPCWSTR deviceID) override
    {
        ComPtr device = [&] {
            auto it = m_deviceMap.find(QString::fromWCharArray(deviceID));
            if (it != std::end(m_deviceMap))
                return it->second.device;

            return ComPtr<IMMDevice>{};
        }();

        if (role == ERole::eMultimedia) {
            switch (flow) {
            case EDataFlow::eCapture:
                emit audioDeviceDefaultChanged(QAudioDevice::Input, device);
                break;
            case EDataFlow::eRender:
                emit audioDeviceDefaultChanged(QAudioDevice::Output, device);
                break;
            case EDataFlow::eAll:
                // Not expected, but handle it anyway
                emit audioDeviceDefaultChanged(QAudioDevice::Input, device);
                emit audioDeviceDefaultChanged(QAudioDevice::Output, device);
                break;
            default:
                Q_UNREACHABLE_RETURN(S_OK);
            }
        }

        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE OnDeviceAdded(LPCWSTR deviceID) override
    {
        auto it = m_deviceMap.find(QString::fromWCharArray(deviceID));
        if (it == std::end(m_deviceMap)) {
            auto enumerateResult = enumerateDevice(deviceID);
            if (!enumerateResult)
                return S_OK;

            m_deviceMap.emplace(QString::fromWCharArray(deviceID), *enumerateResult);

            if (enumerateResult->state == DeviceState::active)
                emit audioDeviceAdded(enumerateResult->device);
        }

        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE OnDeviceRemoved(LPCWSTR deviceID) override
    {
        auto key = QString::fromWCharArray(deviceID);
        auto it = m_deviceMap.find(key);
        if (it != std::end(m_deviceMap)) {
            if (it->second.state == DeviceState::active)
                emit audioDeviceRemoved(it->second.device);
            m_deviceMap.erase(key);
        }

        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE OnDeviceStateChanged(LPCWSTR deviceID, DWORD state) override
    {
        const DeviceState newState = asDeviceState(state);

        if (auto it = m_deviceMap.find(QString::fromWCharArray(deviceID));
            it != std::end(m_deviceMap)) {
            if (it->second.state == newState)
                return S_OK;

            if (newState == DeviceState::active)
                emit audioDeviceAdded(it->second.device);
            else if (newState == DeviceState::active && it->second.state != DeviceState::active)
                emit audioDeviceRemoved(it->second.device);

            it->second.state = newState;
        }

        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE OnPropertyValueChanged(LPCWSTR deviceID, const PROPERTYKEY) override
    {
        if (auto it = m_deviceMap.find(QString::fromWCharArray(deviceID));
            it != std::end(m_deviceMap)) {
            emit audioDevicePropertyChanged(it->second.device);
        }

        return S_OK;
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
    using namespace QtWASAPI;

    auto hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_INPROC_SERVER,
                               IID_PPV_ARGS(&m_deviceEnumerator));

    if (FAILED(hr)) {
        qWarning("Failed to instantiate IMMDeviceEnumerator (%s)."
                 "Audio device change notification will be disabled",
            qPrintable(QSystemError::windowsComString(hr)));
        return;
    }

    m_notificationClient = makeComObject<QtWASAPI::CMMNotificationClient>(m_deviceEnumerator);
    m_deviceEnumerator->RegisterEndpointNotificationCallback(m_notificationClient.Get());

    connect(m_notificationClient.Get(), &QtWASAPI::CMMNotificationClient::audioDeviceAdded, this,
            [this] {
        onAudioInputsChanged();
        onAudioOutputsChanged();
    });
    connect(m_notificationClient.Get(), &QtWASAPI::CMMNotificationClient::audioDeviceRemoved, this,
            [this](ComPtr<IMMDevice> device) {
        {
            std::lock_guard lock(m_cacheMutex);
            m_cachedDevices.erase(device);
        }
        onAudioInputsChanged();
        onAudioOutputsChanged();
    });
    connect(m_notificationClient.Get(), &QtWASAPI::CMMNotificationClient::audioDeviceDefaultChanged,
            this, [this](QAudioDevice::Mode mode, ComPtr<IMMDevice> device) {
        {
            std::lock_guard lock(m_cacheMutex);

            for (auto &entry : m_cachedDevices) {
                if (entry.second.mode() != mode)
                    continue;

                auto handle = QAudioDevicePrivate::handle<QWindowsAudioDevice>(entry.second);
                Q_PRESUME(handle);

                std::unique_ptr<QAudioDevicePrivate> newPrivate = handle->clone();
                newPrivate->isDefault = entry.first == device;

                entry.second = QAudioDevicePrivate::createQAudioDevice(std::move(newPrivate));
            }
        }

        switch (mode) {
        case QAudioDevice::Input:
            onAudioInputsChanged();
            break;
        case QAudioDevice::Output:
            onAudioOutputsChanged();
            break;
        default:
            break;
        }
    });
    connect(m_notificationClient.Get(),
            &QtWASAPI::CMMNotificationClient::audioDevicePropertyChanged, this,
            [this](ComPtr<IMMDevice> device) {
        {
            std::lock_guard lock(m_cacheMutex);
            m_cachedDevices.erase(device);
        }

        onAudioInputsChanged();
        onAudioOutputsChanged();
    });
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

static std::optional<QAudioDevice> asQAudioDevice(ComPtr<IMMDevice> device, QAudioDevice::Mode mode,
                                                  std::optional<QString> defaultAudioDeviceID)
{
    using QtMultimediaPrivate::PropertyStoreHelper;

    std::optional<QString> deviceId = getDeviceId(device);
    if (!deviceId)
        return std::nullopt;

    q23::expected<PropertyStoreHelper, QString> props = PropertyStoreHelper::open(device);
    if (!props) {
        qWarning() << "OpenPropertyStore failed" << props.error();
        return std::nullopt;
    }

    std::optional<QString> friendlyName = props->getString(PKEY_Device_FriendlyName);
    if (!friendlyName) {
        qWarning() << "Cannot read property store";
        return std::nullopt;
    }

    std::optional<QUuid> deviceContainerId = props->getGUID(PKEY_Device_ContainerIdQt);
    if (!deviceContainerId) {
        qWarning() << "Cannot read property store";
        return std::nullopt;
    }

    std::optional<uint32_t> formFactor = props->getUInt32(PKEY_AudioEndpoint_FormFactor);
    if (!formFactor) {
        qWarning() << "Cannot infer form factor";
        return std::nullopt;
    }

    auto dev = std::make_unique<QWindowsAudioDevice>(deviceId->toUtf8(), device, *friendlyName,
                                                     *deviceContainerId,
                                                     EndpointFormFactor(*formFactor), mode);
    dev->isDefault = deviceId == defaultAudioDeviceID;
    return QAudioDevicePrivate::createQAudioDevice(std::move(dev));
}

QList<QAudioDevice> QWindowsAudioDevices::availableDevices(QAudioDevice::Mode mode) const
{
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

        {
            std::lock_guard lock(m_cacheMutex);
            auto cachedDevice = m_cachedDevices.find(device);
            if (cachedDevice != m_cachedDevices.end()) {
                devices.append(cachedDevice->second);
                continue;
            }
        }

        std::optional<QAudioDevice> audioDevice =
                asQAudioDevice(device, mode, defaultAudioDeviceID);

        if (audioDevice) {
            devices.append(*audioDevice);
            std::lock_guard lock(m_cacheMutex);
            m_cachedDevices.emplace(device, *audioDevice);
        }
    }

    auto deviceOrder = [](const QAudioDevice &lhs, const QAudioDevice &rhs) {
        auto lhsHandle = QAudioDevicePrivate::handle<QWindowsAudioDevice>(lhs);
        auto rhsHandle = QAudioDevicePrivate::handle<QWindowsAudioDevice>(rhs);
        auto lhsKey = std::tie(lhsHandle->m_device_ContainerId, lhsHandle->m_formFactor,
                               lhsHandle->description);
        auto rhsKey = std::tie(rhsHandle->m_device_ContainerId, rhsHandle->m_formFactor,
                               rhsHandle->description);
        return lhsKey < rhsKey;
    };

    std::sort(devices.begin(), devices.end(), deviceOrder);
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

#include "qwindowsaudiodevices.moc"
