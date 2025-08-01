// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwindows_wasapi_warmup_client_p.h"

#include <QtCore/qapplicationstatic.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qdebug.h>
#include <QtCore/qspan.h>
#include <QtCore/qthread.h>
#include <QtCore/qtimer.h>
#include <QtCore/quuid.h>
#include <QtCore/private/qsystemerror_p.h>
#include <QtMultimedia/private/qcominitializer_p.h>
#include <QtMultimedia/private/qcomtaskresource_p.h>
#include <QtMultimedia/private/qwindowsaudioutils_p.h>

#include <audioclient.h>
#include <guiddef.h>
#include <mmdeviceapi.h>
#include <powrprof.h>

QT_BEGIN_NAMESPACE

namespace QtMultimediaPrivate {

using namespace std::chrono_literals;

namespace {

///////////////////////////////////////////////////////////////////////////////////////////////////

// The "warm-up" audio client is required to run in the background in order to keep audio engine
// ready for audio output immediately after creating any other subsequent audio client.
class QWasapiWarmupClient
{
public:
    QWasapiWarmupClient();
    ~QWasapiWarmupClient();

private:
    ComPtr<IAudioClient3> m_warmupClient;
};

QWasapiWarmupClient::QWasapiWarmupClient()
{
    using namespace QWindowsAudioUtils;
    ensureComInitializedOnThisThread();

    ComPtr<IMMDeviceEnumerator> deviceEnumerator;
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                                  IID_PPV_ARGS(&deviceEnumerator));
    if (FAILED(hr)) {
        qWarning() << "Failed to create device enumerator" << audioClientErrorString(hr);
        return;
    }

    ComPtr<IMMDevice> device;
    hr = deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, device.GetAddressOf());
    if (FAILED(hr)) {
        if (hr != E_NOTFOUND)
            qWarning() << "Failed to retrieve default audio endpoint" << audioClientErrorString(hr);
        return;
    }

    hr = device->Activate(__uuidof(IAudioClient3), CLSCTX_ALL, nullptr,
                          reinterpret_cast<void **>(m_warmupClient.GetAddressOf()));
    if (FAILED(hr)) {
        qWarning() << "Failed to activate audio engine" << audioClientErrorString(hr);
        return;
    }

    QComTaskResource<WAVEFORMATEX> deviceFormat;
    UINT32 currentPeriodInFrames = 0;
    hr = m_warmupClient->GetCurrentSharedModeEnginePeriod(deviceFormat.address(),
                                                          &currentPeriodInFrames);
    if (FAILED(hr)) {
        qWarning() << "Failed to retrieve the current format and periodicity of the audio engine"
                   << audioClientErrorString(hr);
        return;
    }

    UINT32 defaultPeriodInFrames = 0;
    UINT32 fundamentalPeriodInFrames = 0;
    UINT32 minPeriodInFrames = 0;
    UINT32 maxPeriodInFrames = 0;
    hr = m_warmupClient->GetSharedModeEnginePeriod(deviceFormat.get(), &defaultPeriodInFrames,
                                                   &fundamentalPeriodInFrames, &minPeriodInFrames,
                                                   &maxPeriodInFrames);
    if (FAILED(hr)) {
        qWarning() << "Failed to retrieve the range of periodicities supported by the audio engine"
                   << audioClientErrorString(hr);
        return;
    }

    audioClientSetRole(m_warmupClient, QtMultimediaPrivate::AudioEndpointRole::SoundEffect);

    hr = m_warmupClient->InitializeSharedAudioStream(
            AUDCLNT_SHAREMODE_SHARED, defaultPeriodInFrames, deviceFormat.get(), nullptr);
    if (FAILED(hr)) {
        qWarning() << "Failed to initialize audio engine stream" << audioClientErrorString(hr);
        return;
    }

    hr = m_warmupClient->Start();
    if (FAILED(hr))
        qWarning() << "Failed to start audio engine" << audioClientErrorString(hr);
}

QWasapiWarmupClient::~QWasapiWarmupClient()
{
    using namespace QWindowsAudioUtils;

    if (m_warmupClient) {
        HRESULT hr = m_warmupClient->Stop();
        if (FAILED(hr))
            qWarning() << "Failed to stop audio engine" << audioClientErrorString(hr);
    }
}

} // namespace

///////////////////////////////////////////////////////////////////////////////////////////////////

// https://learn.microsoft.com/en-us/windows-hardware/customize/power-settings/sleep-settings-sleep-idle-timeout
DEFINE_GUID(GUID_SLEEP_TIMEOUT, 0x29f6c1db, 0x86da, 0x48c5, 0x9f, 0xdb, 0xf2, 0xb6, 0x7b, 0x1f,
            0x44, 0xda);

class SleepTimeoutMonitor : public QObject
{
    Q_OBJECT

public:
    SleepTimeoutMonitor();
    ~SleepTimeoutMonitor();

signals:
    void sleepTimeoutChanged(std::chrono::seconds);

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static LRESULT handlePowerSettingsChanged(HWND hwnd, POWERBROADCAST_SETTING *pbs);

    std::wstring m_windowClass;
    HWND m_offscreenWindow{};
    HPOWERNOTIFY m_powerNotifySleepTimeout;
};

SleepTimeoutMonitor::SleepTimeoutMonitor()
{
    m_windowClass = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdWString()
            + L"PowerNotificationWindowClass";

    WNDCLASSEX wc{};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = nullptr;
    wc.lpszClassName = m_windowClass.c_str();
    wc.hCursor = nullptr;

    if (!RegisterClassEx(&wc)) {
        qWarning() << "RegisterClassEx failed" << QSystemError::windowsString();
        m_windowClass.clear();
        return;
    }

    m_offscreenWindow = CreateWindowEx(0, m_windowClass.c_str(), L"Power Notification Window", 0, 0,
                                       0, 0, 0, HWND_MESSAGE, nullptr, nullptr, this);

    if (!m_offscreenWindow) {
        qWarning() << "CreateWindowEx failed" << QSystemError::windowsString();
        return;
    }

    SetWindowLongPtr(m_offscreenWindow, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

    m_powerNotifySleepTimeout = RegisterPowerSettingNotification(
            m_offscreenWindow, &GUID_SLEEP_TIMEOUT, DEVICE_NOTIFY_WINDOW_HANDLE);
    if (!m_powerNotifySleepTimeout) {
        qWarning() << "RegisterPowerSettingNotification failed" << QSystemError::windowsString();
    }
}

SleepTimeoutMonitor::~SleepTimeoutMonitor()
{
    if (m_powerNotifySleepTimeout)
        UnregisterPowerSettingNotification(m_powerNotifySleepTimeout);

    if (m_offscreenWindow)
        DestroyWindow(m_offscreenWindow);
    if (!m_windowClass.empty())
        UnregisterClass(m_windowClass.c_str(), GetModuleHandle(nullptr));
}

LRESULT SleepTimeoutMonitor::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_POWERBROADCAST: {
        switch (wParam) {
        case PBT_POWERSETTINGCHANGE: {
            POWERBROADCAST_SETTING *pbs = reinterpret_cast<POWERBROADCAST_SETTING *>(lParam);
            return handlePowerSettingsChanged(hwnd, pbs);
        }
        default:
            break;
        }
        break;
    }
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    return 0;
}

LRESULT SleepTimeoutMonitor::handlePowerSettingsChanged(HWND hwnd, POWERBROADCAST_SETTING *pbs)
{
    if (pbs->PowerSetting == GUID_SLEEP_TIMEOUT) {
        Q_ASSERT(pbs->DataLength == 4);
        union UnpackUnion {
            UCHAR data[4];
            DWORD sleepTimeoutInSeconds;
        } helper;

        std::copy_n(pbs->Data, pbs->DataLength, helper.data);
        auto self = reinterpret_cast<SleepTimeoutMonitor *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

        emit self->sleepTimeoutChanged(std::chrono::seconds{ helper.sleepTimeoutInSeconds });
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace {

// The wasapi warmup client prevents excessive fade-in times on some systems. However keeping it
// around prevents the system from going to sleep.
// so we don't keep it around forever, but only for half the time before the system may go to sleep
// (or 5 minutes max)
class QWasapiWarmupClientHelper
{
public:
    QWasapiWarmupClientHelper()
    {
        m_cleanupTimer.setInterval(120s);
        m_cleanupTimer.setTimerType(Qt::TimerType::VeryCoarseTimer);
        m_cleanupTimer.setSingleShot(true);
        m_cleanupTimer.callOnTimeout(&m_cleanupTimer, [this] {
            m_warmupClient = {};
        });

        if (!QThread::isMainThread())
            m_cleanupTimer.moveToThread(qApp->thread());

        QObject::connect(qApp, &QCoreApplication::aboutToQuit, qApp, [this] {
            m_warmupClient = {};
        });

        QObject::connect(&m_sleepTimeoutMonitor, &SleepTimeoutMonitor::sleepTimeoutChanged,
                         &m_cleanupTimer, [this](std::chrono::seconds sleepTimeout) {
            m_cleanupTimer.setInterval(std::min<std::chrono::milliseconds>(sleepTimeout / 2, 5min));
        });
    }

    void refresh()
    {
        Q_ASSERT(QThread::isMainThread());

        if (!m_warmupClient)
            m_warmupClient = std::make_unique<QWasapiWarmupClient>();

        m_cleanupTimer.start();
    }

private:
    QTimer m_cleanupTimer;
    SleepTimeoutMonitor m_sleepTimeoutMonitor;
    std::unique_ptr<QWasapiWarmupClient> m_warmupClient;
};

Q_APPLICATION_STATIC(QWasapiWarmupClientHelper, warmupClient);

} // namespace

///////////////////////////////////////////////////////////////////////////////////////////////////

void refreshWarmupClient()
{
    const static bool useWarmupClient = [] {
        bool envVarSet = false;
        int disableWarmup = qEnvironmentVariableIntValue("QT_DISABLE_AUDIO_PREPARE", &envVarSet);
        return !envVarSet || disableWarmup == 0;
    }();

    if (!useWarmupClient)
        return;

    QMetaObject::invokeMethod(qApp, [] {
        warmupClient->refresh();
    });
}

} // namespace QtMultimediaPrivate

QT_END_NAMESPACE

#include "qwindows_wasapi_warmup_client.moc"
