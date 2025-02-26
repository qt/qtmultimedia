// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwindowsaudiodevices_p.h"
#include "qmediadevices.h"
#include "qvarlengtharray.h"

#include "qwindowsaudiosource_p.h"
#include "qwindowsaudiosink_p.h"
#include "qwindowsaudiodevice_p.h"
#include "qcomtaskresource_p.h"

#include <mmsystem.h>
#include <mmddk.h>
#include <mfobjects.h>
#include <mfidl.h>
#include <mferror.h>
#include <mmdeviceapi.h>
#include <qwindowsmfdefs_p.h>

#include <QtCore/qmap.h>
#include <private/qcomobject_p.h>
#include <private/qsystemerror_p.h>

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
    if (m_warmUpAudioClient) {
        HRESULT hr = m_warmUpAudioClient->Stop();
        if (FAILED(hr)) {
            qWarning() << "Failed to stop audio engine" << hr;
        }
    }

    m_deviceEnumerator.Reset();
    m_notificationClient.Reset();
    m_warmUpAudioClient.Reset();
}

namespace {

QString getDeviceId(bool isOutput, UINT waveID)
{
    void *wave = UIntToPtr(waveID);
    auto waveMessage = [wave, isOutput](UINT msg, auto p0, auto p1) {
        return isOutput
                ? waveOutMessage(static_cast<HWAVEOUT>(wave), msg, reinterpret_cast<DWORD_PTR>(p0),
                                 static_cast<DWORD_PTR>(p1))
                : waveInMessage(static_cast<HWAVEIN>(wave), msg, reinterpret_cast<DWORD_PTR>(p0),
                                static_cast<DWORD_PTR>(p1));
    };

    ULONG len = 0; // Includes space for null-terminator
    if (waveMessage(DRV_QUERYFUNCTIONINSTANCEIDSIZE, &len, 0) != MMSYSERR_NOERROR)
        return {};

    QVarLengthArray<WCHAR> deviceId(static_cast<qsizetype>(len / sizeof(WCHAR)));
    if (waveMessage(DRV_QUERYFUNCTIONINSTANCEID, deviceId.data(), len) != MMSYSERR_NOERROR)
        return {};

    Q_ASSERT(deviceId.empty() || deviceId.back() == QChar::Null);

    return QString::fromWCharArray(deviceId.data());
}
}

QList<QAudioDevice> QWindowsAudioDevices::availableDevices(QAudioDevice::Mode mode) const
{
    if (!m_deviceEnumerator)
        return {};

    const bool audioOut = mode == QAudioDevice::Output;

    const auto defaultAudioDeviceID = [this, audioOut]{
        const auto dataFlow = audioOut ? EDataFlow::eRender : EDataFlow::eCapture;
        ComPtr<IMMDevice> dev;
        QComTaskResource<WCHAR> id;
        QString sid;

        if (SUCCEEDED(m_deviceEnumerator->GetDefaultAudioEndpoint(dataFlow, ERole::eMultimedia, dev.GetAddressOf()))) {
            if (dev && SUCCEEDED(dev->GetId(id.address()))) {
                sid = QString::fromWCharArray(id.get());
            }
        }
        return sid.toUtf8();
    }();

    QList<QAudioDevice> devices;

    const UINT waveDevices = audioOut ? waveOutGetNumDevs() : waveInGetNumDevs();

    for (UINT waveID = 0u; waveID < waveDevices; waveID++) {
        const QString deviceId = getDeviceId(audioOut, waveID);
        if (deviceId.isEmpty())
            continue;

        ComPtr<IMMDevice> device;
        ComPtr<IPropertyStore> props;
        if (FAILED(m_deviceEnumerator->GetDevice(deviceId.toStdWString().c_str(),
                                                 device.GetAddressOf()))
            || FAILED(device->OpenPropertyStore(STGM_READ, props.GetAddressOf()))) {
            continue;
        }

        PROPVARIANT varName;
        PropVariantInit(&varName);

        if (SUCCEEDED(props->GetValue(QMM_PKEY_Device_FriendlyName, &varName))) {
            const QString description = QString::fromWCharArray(varName.pwszVal);
            const QByteArray strID = deviceId.toUtf8();

            auto dev = new QWindowsAudioDeviceInfo(strID, device, description, mode);
            dev->isDefault = strID == defaultAudioDeviceID;

            devices.append(dev->create());
        }
        PropVariantClear(&varName);
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

QPlatformAudioSource *QWindowsAudioDevices::createAudioSource(const QAudioDevice &deviceInfo,
                                                              const QAudioFormat &fmt,
                                                              QObject *parent)
{
    const auto *devInfo = static_cast<const QWindowsAudioDeviceInfo *>(deviceInfo.handle());
    return new QWindowsAudioSource(devInfo->immDev(), fmt, parent);
}

QPlatformAudioSink *QWindowsAudioDevices::createAudioSink(const QAudioDevice &deviceInfo,
                                                          const QAudioFormat &fmt,
                                                          QObject *parent)
{
    const auto *devInfo = static_cast<const QWindowsAudioDeviceInfo *>(deviceInfo.handle());
    return new QWindowsAudioSink(devInfo->immDev(), fmt, parent);
}

static bool isPrepareAudioEnabled()
{
    static bool isDisableAudioPrepareSet = false;
    static const int disableAudioPrepare =
            qEnvironmentVariableIntValue("QT_DISABLE_AUDIO_PREPARE", &isDisableAudioPrepareSet);

    return !isDisableAudioPrepareSet || disableAudioPrepare == 0;
}

void QWindowsAudioDevices::prepareAudio()
{
    if (!isPrepareAudioEnabled())
        return;

    if (m_isAudioClientWarmedUp.exchange(true))
        return;

    ComPtr<IMMDeviceEnumerator> deviceEnumerator;
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                                  __uuidof(IMMDeviceEnumerator),
                                  reinterpret_cast<void **>(deviceEnumerator.GetAddressOf()));
    if (FAILED(hr)) {
        qWarning() << "Failed to create device enumerator" << hr;
        return;
    }

    ComPtr<IMMDevice> device;
    hr = deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, device.GetAddressOf());
    if (FAILED(hr)) {
        if (hr != E_NOTFOUND)
            qWarning() << "Failed to retrieve default audio endpoint" << hr;
        return;
    }

    hr = device->Activate(__uuidof(IAudioClient3), CLSCTX_ALL, nullptr,
                          reinterpret_cast<void **>(m_warmUpAudioClient.GetAddressOf()));
    if (FAILED(hr)) {
        qWarning() << "Failed to activate audio engine" << hr;
        return;
    }

    QComTaskResource<WAVEFORMATEX> deviceFormat;
    UINT32 currentPeriodInFrames = 0;
    hr = m_warmUpAudioClient->GetCurrentSharedModeEnginePeriod(deviceFormat.address(),
                                                               &currentPeriodInFrames);
    if (FAILED(hr)) {
        qWarning() << "Failed to retrieve the current format and periodicity of the audio engine"
                   << hr;
        return;
    }

    UINT32 defaultPeriodInFrames = 0;
    UINT32 fundamentalPeriodInFrames = 0;
    UINT32 minPeriodInFrames = 0;
    UINT32 maxPeriodInFrames = 0;
    hr = m_warmUpAudioClient->GetSharedModeEnginePeriod(deviceFormat.get(), &defaultPeriodInFrames,
                                                        &fundamentalPeriodInFrames,
                                                        &minPeriodInFrames, &maxPeriodInFrames);
    if (FAILED(hr)) {
        qWarning() << "Failed to retrieve the range of periodicities supported by the audio engine"
                   << hr;
        return;
    }

    hr = m_warmUpAudioClient->InitializeSharedAudioStream(
            AUDCLNT_SHAREMODE_SHARED, minPeriodInFrames, deviceFormat.get(), nullptr);
    if (FAILED(hr)) {
        qWarning() << "Failed to initialize audio engine stream" << hr;
        return;
    }

    hr = m_warmUpAudioClient->Start();
    if (FAILED(hr))
        qWarning() << "Failed to start audio engine" << hr;
}

QT_END_NAMESPACE
