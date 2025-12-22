// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qdarwinaudiodevices_p.h"

#include <QtMultimedia/qmediadevices.h>
#include <QtMultimedia/private/qaudiodevice_p.h>
#include <QtMultimedia/private/qdarwinaudiodevice_p.h>
#include <QtMultimedia/private/qdarwinaudiosink_p.h>
#include <QtMultimedia/private/qdarwinaudiosource_p.h>

#include <QtCore/qcoreapplication.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qdebug.h>

#if defined(QT_PLATFORM_UIKIT)
#  include <QtMultimedia/private/qcoreaudiosessionmanager_p.h>
#  import <AVFoundation/AVFoundation.h>
#else

#  include <QtMultimedia/private/qmacosaudiodatautils_p.h>
#endif

#if defined(Q_OS_MACOS)
Q_STATIC_LOGGING_CATEGORY(qLcDarwinMediaDevices, "qt.multimedia.darwin.mediaDevices");
#endif

QT_BEGIN_NAMESPACE

template <typename... Args>
static QAudioDevice createAudioDevice(bool isDefault, Args &&...args)
{
    auto dev = std::make_unique<QCoreAudioDeviceInfo>(std::forward<Args>(args)...);
    dev->isDefault = isDefault;
    return QAudioDevicePrivate::createQAudioDevice(std::move(dev));
}

#if defined(Q_OS_MACOS)

static AudioDeviceID defaultAudioDevice(QAudioDevice::Mode mode)
{
    const AudioObjectPropertySelector selector = (mode == QAudioDevice::Output)
            ? kAudioHardwarePropertyDefaultOutputDevice
            : kAudioHardwarePropertyDefaultInputDevice;
    const AudioObjectPropertyAddress propertyAddress = {
        selector,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMain,
    };

    if (auto audioDevice = QCoreAudioUtils::getAudioProperty<AudioDeviceID>(kAudioObjectSystemObject, propertyAddress)) {
        return *audioDevice;
    }

    return 0;
}

static QList<QAudioDevice> availableAudioDevices(QAudioDevice::Mode mode)
{
    using namespace QCoreAudioUtils;

    QList<QAudioDevice> devices;

    AudioDeviceID defaultDevice = defaultAudioDevice(mode);
    if (defaultDevice != 0)
        devices << createAudioDevice(
            true,
            defaultDevice,
            QCoreAudioUtils::readPersistentDeviceId(defaultDevice, mode),
            mode);

    const AudioObjectPropertyAddress audioDevicesPropertyAddress = {
        kAudioHardwarePropertyDevices, kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMain
    };

    if (auto audioDevices = getAudioPropertyList<AudioDeviceID>(
                kAudioObjectSystemObject, audioDevicesPropertyAddress)) {
        const AudioObjectPropertyAddress audioDeviceStreamFormatPropertyAddress =
                makePropertyAddress(kAudioDevicePropertyStreamFormat, mode);

        for (const auto &device : *audioDevices) {
            if (device == defaultDevice)
                continue;

            if (getAudioProperty<AudioStreamBasicDescription>(device,
                                                            audioDeviceStreamFormatPropertyAddress,
                                                            /*warnIfMissing=*/false)) {
                devices << createAudioDevice(false,
                                             device,
                                             QCoreAudioUtils::readPersistentDeviceId(device, mode),
                                             mode);
            }
        }
    }

    return devices;
}

#elif defined(QT_PLATFORM_UIKIT)

static QList<QAudioDevice> availableAudioDevices(QAudioDevice::Mode mode)
{
    QList<QAudioDevice> devices;

    if (mode == QAudioDevice::Output) {
        devices.append(createAudioDevice(true, "default", QAudioDevice::Output));
    } else {
#if !defined(Q_OS_VISIONOS)
        AVCaptureDevice *defaultDevice =
                [AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeAudio];

        // TODO: Support Bluetooth and USB devices
        AVCaptureDeviceDiscoverySession *captureDeviceDiscoverySession =
            [AVCaptureDeviceDiscoverySession
                discoverySessionWithDeviceTypes:@[ AVCaptureDeviceTypeMicrophone ]
                                      mediaType:AVMediaTypeAudio
                                       position:AVCaptureDevicePositionUnspecified];

        NSArray *captureDevices = [captureDeviceDiscoverySession devices];
        for (AVCaptureDevice *device in captureDevices) {
            const bool isDefault =
                    defaultDevice && [defaultDevice.uniqueID isEqualToString:device.uniqueID];
            devices.append(createAudioDevice(isDefault,
                                             QString::fromNSString(device.uniqueID).toUtf8(),
                                             QAudioDevice::Input));
        }
#endif
    }

    return devices;
}

#endif

#ifdef Q_OS_MACOS

static constexpr AudioObjectPropertyAddress listenerAddresses[] = {
    { kAudioHardwarePropertyDefaultInputDevice, kAudioObjectPropertyScopeGlobal,
      kAudioObjectPropertyElementMain },
    { kAudioHardwarePropertyDefaultOutputDevice, kAudioObjectPropertyScopeGlobal,
      kAudioObjectPropertyElementMain },
    { kAudioHardwarePropertyDevices, kAudioObjectPropertyScopeGlobal,
      kAudioObjectPropertyElementMain }
};

#endif

QDarwinAudioDevices::QDarwinAudioDevices()
{
    if (!QThread::isMainThread())
        moveToThread(qApp->thread());

#ifdef Q_OS_MACOS
    m_listenerQueue.reset(
            dispatch_queue_create("QtAudioDevicesListener", /*dispatch_queue_attr=*/nullptr));
    dispatch_set_target_queue(m_listenerQueue.get(), dispatch_get_main_queue());

    std::shared_ptr destroyedFlag = m_destroyed; // capture shared_ptr
    m_deviceListenerBlock = std::make_unique<AudioObjectPropertyListenerBlock>(
            [this, destroyedFlag](UInt32 numberOfProps, const AudioObjectPropertyAddress *props) {
        // the block is dispatched via a dispatch queue on the main thread and can be in-flight when
        // the QDarwinAudioDevices instance is being destroyed. Check the destroyed flag to avoid
        // accessing deleted memory.
        if (*destroyedFlag)
            return;

        Q_ASSERT(QThread::isMainThread());

        auto properties = QSpan{ props, numberOfProps };

        bool updateInputs = false;
        bool updateOutputs = false;

        for (const AudioObjectPropertyAddress &address : properties) {
            qCDebug(qLcDarwinMediaDevices)
                    << "audioDeviceChangeListenerBlock: address: " << address.mSelector
                    << address.mScope << address.mElement;

            switch (address.mSelector) {
            case kAudioHardwarePropertyDefaultInputDevice:
                updateInputs = true;
                break;
            case kAudioHardwarePropertyDefaultOutputDevice:
                updateOutputs = true;
                break;
            default:
                updateInputs = true;
                updateOutputs = true;
                break;
            }
        }

        if (updateInputs)
            updateAudioInputsCache();
        if (updateOutputs)
            updateAudioOutputsCache();
    });

    for (const auto &address : listenerAddresses) {
        const auto err = AudioObjectAddPropertyListenerBlock(
                kAudioObjectSystemObject, &address, m_listenerQueue.get(), *m_deviceListenerBlock);
        if (err)
            qWarning() << "Fail to add listener. mSelector:" << address.mSelector
                       << "mScope:" << address.mScope << "mElement:" << address.mElement
                       << "err:" << err;
    }

    updateAudioInputsCache();
    updateAudioOutputsCache();
#endif
}

QDarwinAudioDevices::~QDarwinAudioDevices()
{
#ifdef Q_OS_MACOS
    *m_destroyed = true;
    for (const auto &address : listenerAddresses) {
        Q_ASSERT(m_deviceListenerBlock);
        AudioObjectRemovePropertyListenerBlock(kAudioObjectSystemObject, &address,
                                               m_listenerQueue.get(), *m_deviceListenerBlock);
    }
    m_listenerQueue = {};
#endif
}

QList<QAudioDevice> QDarwinAudioDevices::findAudioInputs() const
{
    return availableAudioDevices(QAudioDevice::Input);
}

QList<QAudioDevice> QDarwinAudioDevices::findAudioOutputs() const
{
    return availableAudioDevices(QAudioDevice::Output);
}

QPlatformAudioSource *QDarwinAudioDevices::createAudioSource(const QAudioDevice &info,
                                                             const QAudioFormat &fmt,
                                                             QObject *parent)
{
    return new QDarwinAudioSource(info, fmt, parent);
}

QPlatformAudioSink *QDarwinAudioDevices::createAudioSink(const QAudioDevice &info,
                                                         const QAudioFormat &fmt, QObject *parent)
{
    return new QDarwinAudioSink(info, fmt, parent);
}

namespace QCoreAudioUtils {

#ifdef Q_OS_MACOS

static constexpr AudioObjectPropertyAddress propertyAddressDeviceIsAlive = {
    kAudioDevicePropertyDeviceIsAlive,
    kAudioObjectPropertyScopeGlobal,
    kAudioObjectPropertyElementMain,
};

// force dtor in a translation unit with ARC enabled
DeviceDisconnectMonitor::~DeviceDisconnectMonitor()
{
    Q_ASSERT(!m_disconnectFunction);
}

std::optional<QFuture<void>> DeviceDisconnectMonitor::addDisconnectListener(AudioObjectID id)
{
    Q_ASSERT(!m_disconnectFunction);

    auto disconnectedPromise = std::make_shared<QPromise<void>>();
    QFuture<void> disconnectFuture = disconnectedPromise->future();

    auto listenerBlock = std::make_shared<AudioObjectPropertyListenerBlock>(
            [disconnectedPromise](UInt32 numberOfProps, const AudioObjectPropertyAddress *props) {
        // Called on HAL thread
        auto properties = QSpan{ props, numberOfProps };

        for (const AudioObjectPropertyAddress &address : properties) {
            if (address.mSelector == kAudioDevicePropertyDeviceIsAlive) {
                disconnectedPromise->start();
                disconnectedPromise->finish();
                return;
            }
        }
    });

    OSStatus status =
            AudioObjectAddPropertyListenerBlock(id, &propertyAddressDeviceIsAlive,
                                                /*inDispatchQueue=*/nullptr, *listenerBlock);

    if (status != noErr) {
        qWarning() << "QAudioOutput: Failed to add property listener";
        return std::nullopt;
    }

    m_disconnectFunction = [id, listenerBlock] {
        AudioObjectRemovePropertyListenerBlock(id, &propertyAddressDeviceIsAlive,
                                               /*inDispatchQueue=*/nullptr, *listenerBlock);
    };

    return disconnectFuture;
}

void DeviceDisconnectMonitor::removeDisconnectListener()
{
    if (!m_disconnectFunction)
        return;
    m_disconnectFunction();
    m_disconnectFunction = nullptr;
}

#endif

} // namespace QCoreAudioUtils

QT_END_NAMESPACE
