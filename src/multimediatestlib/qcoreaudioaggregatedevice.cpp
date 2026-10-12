// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qcoreaudioaggregatedevice_p.h"

#include <QtMultimedia/qmediadevices.h>
#include <QtMultimedia/private/qmultimedia_ranges_p.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qdeadlinetimer.h>
#include <QtCore/qthread.h>
#include <QtCore/quuid.h>
#include <QtCore/private/qcore_mac_p.h>

#include <CoreAudio/CoreAudio.h>
#include <CoreFoundation/CoreFoundation.h>

#include <atomic>
#include <optional>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;
using namespace std::chrono_literals;

namespace {

constexpr AudioObjectPropertyAddress isAliveAddress{
    kAudioDevicePropertyDeviceIsAlive,
    kAudioObjectPropertyScopeGlobal,
    kAudioObjectPropertyElementMain,
};

std::optional<QString> defaultDeviceUid(QCoreAudioAggregateDevice::Direction direction)
{
    const AudioObjectPropertyAddress defaultDeviceAddress{
        direction == QCoreAudioAggregateDevice::Direction::Output
                ? kAudioHardwarePropertyDefaultOutputDevice
                : kAudioHardwarePropertyDefaultInputDevice,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMain,
    };

    AudioObjectID deviceId = kAudioObjectUnknown;
    UInt32 size = sizeof(deviceId);
    if (AudioObjectGetPropertyData(kAudioObjectSystemObject, &defaultDeviceAddress, 0, nullptr,
                                   &size, &deviceId)
                != noErr
        || deviceId == kAudioObjectUnknown)
        return std::nullopt;

    const AudioObjectPropertyAddress uidAddress{
        kAudioDevicePropertyDeviceUID,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMain,
    };
    CFStringRef uid = nullptr;
    size = sizeof(uid);
    if (AudioObjectGetPropertyData(deviceId, &uidAddress, 0, nullptr, &size, &uid) != noErr || !uid)
        return std::nullopt;

    QCFType<CFStringRef> ownedUid{ uid };
    return QString::fromCFString(ownedUid);
}

OSStatus onPropertyChanged(AudioObjectID, UInt32 numberOfAddresses,
                           const AudioObjectPropertyAddress *addresses, void *clientData)
{
    auto *notifications = static_cast<std::atomic<int> *>(clientData);
    for (UInt32 index = 0; index != numberOfAddresses; ++index) {
        if (addresses[index].mSelector == kAudioDevicePropertyDeviceIsAlive)
            notifications->fetch_add(1);
    }
    return noErr;
}

} // namespace

struct QCoreAudioAggregateDevice::Private
{
    ~Private()
    {
        if (id == kAudioObjectUnknown)
            return;
        if (listenerInstalled)
            AudioObjectRemovePropertyListener(id, &isAliveAddress, onPropertyChanged,
                                              &isAliveNotifications);
        AudioHardwareDestroyAggregateDevice(id);
    }

    AudioObjectID id = kAudioObjectUnknown;
    QString name;
    std::atomic<int> isAliveNotifications = 0;
    bool listenerInstalled = false;
};

QCoreAudioAggregateDevice::QCoreAudioAggregateDevice(std::unique_ptr<Private> priv)
    : d{ std::move(priv) }
{
}

QCoreAudioAggregateDevice::~QCoreAudioAggregateDevice() = default;

std::unique_ptr<QCoreAudioAggregateDevice> QCoreAudioAggregateDevice::create(Direction direction)
{
    const std::optional<QString> subDeviceUid = defaultDeviceUid(direction);
    if (!subDeviceUid)
        return nullptr;

    // Fresh name/UID per attempt: parallel ctest processes create aggregates concurrently,
    // so a reused UID could collide with a device owned by another test process.
    constexpr int maxAttempts = 3;
    for (int attempt = 0; attempt != maxAttempts; ++attempt) {
        const QString uniqueSuffix = QUuid::createUuid().toString(QUuid::Id128);
        const QString name = u"QtTestAggregate"_s + uniqueSuffix;
        const QString uid = u"io.qt.test.aggregate."_s + uniqueSuffix;

        QCFType<CFMutableDictionaryRef> description{ CFDictionaryCreateMutable(
                kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks,
                &kCFTypeDictionaryValueCallBacks) };
        QCFType<CFStringRef> cfName = name.toCFString();
        QCFType<CFStringRef> cfUid = uid.toCFString();
        QCFType<CFStringRef> cfSubDeviceUid = subDeviceUid->toCFString();
        const int isPrivate = 1;
        QCFType<CFNumberRef> cfIsPrivate =
                CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &isPrivate);

        QCFType<CFMutableDictionaryRef> subDevice{
            CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks,
                                      &kCFTypeDictionaryValueCallBacks),
        };
        CFDictionaryAddValue(subDevice, CFSTR(kAudioSubDeviceUIDKey), cfSubDeviceUid);
        QCFType<CFMutableArrayRef> subDevices{
            CFArrayCreateMutable(kCFAllocatorDefault, 1, &kCFTypeArrayCallBacks),
        };
        CFArrayAppendValue(subDevices, subDevice);

        CFDictionaryAddValue(description, CFSTR(kAudioAggregateDeviceNameKey), cfName);
        CFDictionaryAddValue(description, CFSTR(kAudioAggregateDeviceUIDKey), cfUid);
        CFDictionaryAddValue(description, CFSTR(kAudioAggregateDeviceIsPrivateKey), cfIsPrivate);
        CFDictionaryAddValue(description, CFSTR(kAudioAggregateDeviceSubDeviceListKey), subDevices);

        AudioObjectID id = kAudioObjectUnknown;
        if (AudioHardwareCreateAggregateDevice(description, &id) == noErr
            && id != kAudioObjectUnknown) {
            auto priv = std::make_unique<Private>();
            priv->id = id;
            priv->name = name;

            return std::unique_ptr<QCoreAudioAggregateDevice>{
                new QCoreAudioAggregateDevice{ std::move(priv) },
            };
        }
    }

    return nullptr;
}

std::optional<QAudioDevice>
QCoreAudioAggregateDevice::waitForQtDevice(std::chrono::milliseconds timeout) const
{
    namespace ranges = QtMultimediaPrivate::ranges;

    QDeadlineTimer deadline{ timeout };
    do {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 5);

        for (auto devices : { QMediaDevices::audioOutputs(), QMediaDevices::audioInputs() }) {
            const auto it = ranges::find_if(devices, [&](const QAudioDevice &device) {
                return device.description() == d->name;
            });
            if (it != devices.end())
                return *it;
        }

        std::this_thread::sleep_for(2ms);
    } while (!deadline.hasExpired());

    return std::nullopt;
}

bool QCoreAudioAggregateDevice::destroyAndWaitForDisconnectNotification(
        std::chrono::milliseconds timeout)
{
    const AudioObjectID id = d->id;
    if (id == kAudioObjectUnknown)
        return false;

    d->isAliveNotifications = 0;

    if (AudioObjectAddPropertyListener(id, &isAliveAddress, onPropertyChanged,
                                       &d->isAliveNotifications)
        != noErr)
        return false;
    d->listenerInstalled = true;

    if (AudioHardwareDestroyAggregateDevice(id) != noErr) {
        AudioObjectRemovePropertyListener(id, &isAliveAddress, onPropertyChanged,
                                          &d->isAliveNotifications);
        d->listenerInstalled = false;
        return false;
    }
    // The HAL drops listeners with the destroyed device
    d->id = kAudioObjectUnknown;
    d->listenerInstalled = false;

    // Block without running the Qt event loop, so QtMultimedia's disconnect handling stays queued
    QDeadlineTimer deadline{ timeout };
    while (d->isAliveNotifications.load() == 0) {
        if (deadline.hasExpired())
            return false;
        std::this_thread::sleep_for(1ms);
    }

    // The HAL notifies listeners in registration order on one thread. QtMultimedia's listener
    // fulfills the promise that posts the continuation, so wait until it has run.
    std::this_thread::sleep_for(20ms);
    return true;
}

QT_END_NAMESPACE
