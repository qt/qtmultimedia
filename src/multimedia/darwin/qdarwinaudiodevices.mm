// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qdarwinaudiodevices_p.h"
#include "qmediadevices.h"
#include "private/qaudiodevice_p.h"
#include "qdarwinaudiodevice_p.h"
#include "qdarwinaudiosource_p.h"
#include "qdarwinaudiosink_p.h"

#include <qloggingcategory.h>

#include <qdebug.h>

#if defined(Q_OS_IOS)
#include "qcoreaudiosessionmanager_p.h"
#import <AVFoundation/AVFoundation.h>
#else
#include "qmacosaudiodatautils_p.h"
#endif

#if defined(Q_OS_MACOS)
static Q_LOGGING_CATEGORY(qLcDarwinMediaDevices, "qt.multimedia.darwin.mediaDevices")
#endif

QT_BEGIN_NAMESPACE

template<typename... Args>
QAudioDevice createAudioDevice(bool isDefault, Args &&...args)
{
    auto *dev = new QCoreAudioDeviceInfo(std::forward<Args>(args)...);
    dev->isDefault = isDefault;
    return dev->create();
}

#if defined(Q_OS_MACOS)

static AudioDeviceID defaultAudioDevice(QAudioDevice::Mode mode)
{
    const AudioObjectPropertySelector selector = (mode == QAudioDevice::Output) ? kAudioHardwarePropertyDefaultOutputDevice
                                                                               : kAudioHardwarePropertyDefaultInputDevice;
    const AudioObjectPropertyAddress propertyAddress = {
        selector,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMain,
    };

    if (auto audioDevice = getAudioObject<AudioDeviceID>(kAudioObjectSystemObject, propertyAddress)) {
        return *audioDevice;
    }

    return 0;
}

static QByteArray uniqueId(AudioDeviceID device, QAudioDevice::Mode mode)
{
    const AudioObjectPropertyAddress propertyAddress =
            makePropertyAddress(kAudioDevicePropertyDeviceUID, mode);

    if (auto name = getAudioObject<QCFString>(device, propertyAddress))
        return QString{*name}.toUtf8();

    return QByteArray();
}

static QList<QAudioDevice> availableAudioDevices(QAudioDevice::Mode mode)
{
    QList<QAudioDevice> devices;

    AudioDeviceID defaultDevice = defaultAudioDevice(mode);
    if (defaultDevice != 0)
        devices << createAudioDevice(true, defaultDevice, uniqueId(defaultDevice, mode), mode);

    const AudioObjectPropertyAddress audioDevicesPropertyAddress = {
        kAudioHardwarePropertyDevices, kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMain
    };

    if (auto audioDevices = getAudioData<AudioDeviceID>(
                kAudioObjectSystemObject, audioDevicesPropertyAddress)) {
        const AudioObjectPropertyAddress audioDeviceStreamFormatPropertyAddress =
                makePropertyAddress(kAudioDevicePropertyStreamFormat, mode);

        for (const auto &device : *audioDevices) {
            if (device == defaultDevice)
                continue;

            if (getAudioObject<AudioStreamBasicDescription>(device,
                                                            audioDeviceStreamFormatPropertyAddress,
                                                            /*warnIfMissing=*/false)) {
                devices << createAudioDevice(false, device, uniqueId(device, mode), mode);
            }
        }
    }

    return devices;
}

static OSStatus audioDeviceChangeListener(AudioObjectID id, UInt32,
                                          const AudioObjectPropertyAddress *address, void *ptr)
{
    Q_ASSERT(address);
    Q_ASSERT(ptr);

    QDarwinAudioDevices *instance = static_cast<QDarwinAudioDevices *>(ptr);

    qCDebug(qLcDarwinMediaDevices)
            << "audioDeviceChangeListener: id:" << id << "address: " << address->mSelector
            << address->mScope << address->mElement;

    switch (address->mSelector) {
    case kAudioHardwarePropertyDefaultInputDevice:
        instance->updateAudioInputsCache();
        break;
    case kAudioHardwarePropertyDefaultOutputDevice:
        instance->updateAudioOutputsCache();
        break;
    default:
        instance->updateAudioInputsCache();
        instance->updateAudioOutputsCache();
        break;
    }

    return 0;
}

static constexpr AudioObjectPropertyAddress listenerAddresses[] = {
    { kAudioHardwarePropertyDefaultInputDevice, kAudioObjectPropertyScopeGlobal,
      kAudioObjectPropertyElementMain },
    { kAudioHardwarePropertyDefaultOutputDevice, kAudioObjectPropertyScopeGlobal,
      kAudioObjectPropertyElementMain },
    { kAudioHardwarePropertyDevices, kAudioObjectPropertyScopeGlobal,
      kAudioObjectPropertyElementMain }
};

static void setAudioListeners(QDarwinAudioDevices &instance)
{
    for (const auto &address : listenerAddresses) {
        const auto err = AudioObjectAddPropertyListener(kAudioObjectSystemObject, &address,
                                                        audioDeviceChangeListener, &instance);

        if (err)
            qWarning() << "Fail to add listener. mSelector:" << address.mSelector
                       << "mScope:" << address.mScope << "mElement:" << address.mElement
                       << "err:" << err;
    }
}

static void removeAudioListeners(QDarwinAudioDevices &instance)
{
    for (const auto &address : listenerAddresses) {
        const auto err = AudioObjectRemovePropertyListener(kAudioObjectSystemObject, &address,
                                                           audioDeviceChangeListener, &instance);

        if (err)
            qWarning() << "Fail to remove listener. mSelector:" << address.mSelector
                       << "mScope:" << address.mScope << "mElement:" << address.mElement
                       << "err:" << err;
    }
}

#elif defined(Q_OS_IOS)

static QList<QAudioDevice> availableAudioDevices(QAudioDevice::Mode mode)
{
    QList<QAudioDevice> devices;

    if (mode == QAudioDevice::Output) {
        devices.append(createAudioDevice(true, "default", QAudioDevice::Output));
    } else {
        AVCaptureDevice *defaultDevice =
                [AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeAudio];

        // TODO: Support Bluetooth and USB devices
        AVCaptureDeviceDiscoverySession *captureDeviceDiscoverySession =
                [AVCaptureDeviceDiscoverySession
                        discoverySessionWithDeviceTypes:@[ AVCaptureDeviceTypeBuiltInMicrophone ]
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
    }

    return devices;
}

static void setAudioListeners(QDarwinAudioDevices &)
{
    // ### This should use the audio session manager
}

static void removeAudioListeners(QDarwinAudioDevices &)
{
    // ### This should use the audio session manager
}

#endif


QDarwinAudioDevices::QDarwinAudioDevices()
    : QPlatformAudioDevices()
{
#ifdef Q_OS_MACOS // TODO: implement setAudioListeners, removeAudioListeners for Q_OS_IOS, after
                  // that - remove or modify the define
    updateAudioInputsCache();
    updateAudioOutputsCache();
#endif

    setAudioListeners(*this);
}


QDarwinAudioDevices::~QDarwinAudioDevices()
{
    removeAudioListeners(*this);
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
                                                             QObject *parent)
{
    return new QDarwinAudioSource(info, parent);
}

QPlatformAudioSink *QDarwinAudioDevices::createAudioSink(const QAudioDevice &info,
                                                         QObject *parent)
{
    return new QDarwinAudioSink(info, parent);
}

QT_END_NAMESPACE
