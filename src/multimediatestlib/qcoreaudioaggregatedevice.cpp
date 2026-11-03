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
#include <cerrno>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

#include <signal.h>
#include <sys/types.h>
#include <unistd.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;
using namespace std::chrono_literals;

namespace {

constexpr AudioObjectPropertyAddress isAliveAddress{
    kAudioDevicePropertyDeviceIsAlive,
    kAudioObjectPropertyScopeGlobal,
    kAudioObjectPropertyElementMain,
};

// UID format: <prefix><owner pid>.<identity>
QString aggregateUidPrefix()
{
    return u"io.qt.test.aggregate."_s;
}

bool isProcessAlive(qint64 pid)
{
    // EPERM: the process exists, but belongs to another user
    return kill(static_cast<pid_t>(pid), 0) == 0 || errno == EPERM;
}

std::optional<QString> deviceUid(AudioObjectID deviceId)
{
    const AudioObjectPropertyAddress uidAddress{
        kAudioDevicePropertyDeviceUID,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMain,
    };
    CFStringRef uid = nullptr;
    UInt32 size = sizeof(uid);
    if (AudioObjectGetPropertyData(deviceId, &uidAddress, 0, nullptr, &size, &uid) != noErr || !uid)
        return std::nullopt;

    QCFType<CFStringRef> ownedUid{ uid };
    return QString::fromCFString(ownedUid);
}

AudioObjectPropertySelector defaultDeviceSelector(QCoreAudioAggregateDevice::Direction direction)
{
    return direction == QCoreAudioAggregateDevice::Direction::Output
            ? kAudioHardwarePropertyDefaultOutputDevice
            : kAudioHardwarePropertyDefaultInputDevice;
}

std::optional<AudioObjectID> defaultDeviceId(AudioObjectPropertySelector selector)
{
    const AudioObjectPropertyAddress defaultDeviceAddress{
        selector,
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

    return deviceId;
}

bool setDefaultDeviceId(AudioObjectPropertySelector selector, AudioObjectID deviceId)
{
    const AudioObjectPropertyAddress defaultDeviceAddress{
        selector,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMain,
    };
    return AudioObjectSetPropertyData(kAudioObjectSystemObject, &defaultDeviceAddress, 0, nullptr,
                                      sizeof(deviceId), &deviceId)
            == noErr;
}

std::optional<QString> defaultDeviceUid(QCoreAudioAggregateDevice::Direction direction)
{
    const std::optional<AudioObjectID> deviceId = defaultDeviceId(defaultDeviceSelector(direction));
    if (!deviceId)
        return std::nullopt;
    return deviceUid(*deviceId);
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
        if (previousDefaultId)
            setDefaultDeviceId(defaultSelector, *previousDefaultId);

        if (id == kAudioObjectUnknown)
            return;
        if (listenerInstalled)
            AudioObjectRemovePropertyListener(id, &isAliveAddress, onPropertyChanged,
                                              &isAliveNotifications);
        AudioHardwareDestroyAggregateDevice(id);
    }

    AudioObjectPropertySelector defaultSelector{};
    AudioObjectID id = kAudioObjectUnknown;
    QString name;
    std::atomic<int> isAliveNotifications = 0;
    bool listenerInstalled = false;
    std::optional<AudioObjectID> previousDefaultId;
};

QCoreAudioAggregateDevice::QCoreAudioAggregateDevice(std::unique_ptr<Private> priv)
    : d{ std::move(priv) }
{
}

QCoreAudioAggregateDevice::~QCoreAudioAggregateDevice() = default;

std::unique_ptr<QCoreAudioAggregateDevice> QCoreAudioAggregateDevice::create(Direction direction,
                                                                             Visibility visibility)
{
    // Fresh identity per attempt: parallel ctest processes create aggregates concurrently,
    // so a reused UID could collide with a device owned by another test process.
    constexpr int maxAttempts = 3;
    for (int attempt = 0; attempt != maxAttempts; ++attempt) {
        if (auto device = create(direction, QUuid::createUuid().toString(QUuid::Id128), visibility))
            return device;
    }

    return nullptr;
}

std::unique_ptr<QCoreAudioAggregateDevice>
QCoreAudioAggregateDevice::create(Direction direction, const QString &identity,
                                  Visibility visibility)
{
    return createOwnedBy(direction, getpid(), identity, visibility);
}

void QCoreAudioAggregateDevice::removeStaleDevices()
{
    const AudioObjectPropertyAddress devicesAddress{
        kAudioHardwarePropertyDevices,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMain,
    };

    UInt32 size = 0;
    if (AudioObjectGetPropertyDataSize(kAudioObjectSystemObject, &devicesAddress, 0, nullptr, &size)
        != noErr)
        return;

    std::vector<AudioObjectID> deviceIds(size / sizeof(AudioObjectID));
    if (AudioObjectGetPropertyData(kAudioObjectSystemObject, &devicesAddress, 0, nullptr, &size,
                                   deviceIds.data())
        != noErr)
        return;

    bool destroyed = false;
    for (const AudioObjectID deviceId : deviceIds) {
        const std::optional<QString> uid = deviceUid(deviceId);
        if (!uid || !uid->startsWith(aggregateUidPrefix()))
            continue;

        // Devices without an owner pid predate the owner encoding, so they are stale too
        bool hasOwner = false;
        const qint64 owner =
                uid->mid(aggregateUidPrefix().size()).section(u'.', 0, 0).toLongLong(&hasOwner);
        if (hasOwner && isProcessAlive(owner))
            continue;

        destroyed |= AudioHardwareDestroyAggregateDevice(deviceId) == noErr;
    }

    if (!destroyed)
        return;

    // Destroying the default device makes CoreAudio pick a new default, but not immediately.
    // A new aggregate built on the old default would have a dead sub-device
    QDeadlineTimer deadline{ 2s };
    for (const Direction direction : { Direction::Output, Direction::Input }) {
        while (true) {
            const std::optional<QString> defaultUid = defaultDeviceUid(direction);
            if (defaultUid && !defaultUid->startsWith(aggregateUidPrefix()))
                break;
            if (deadline.hasExpired())
                return;
            std::this_thread::sleep_for(10ms);
        }
    }
}

std::unique_ptr<QCoreAudioAggregateDevice>
QCoreAudioAggregateDevice::createOwnedBy(Direction direction, qint64 ownerPid,
                                         const QString &identity, Visibility visibility)
{
    static std::once_flag staleDevicesRemoved;
    std::call_once(staleDevicesRemoved, removeStaleDevices);

    const std::optional<QString> subDeviceUid = defaultDeviceUid(direction);
    if (!subDeviceUid)
        return nullptr;

    const QString name = u"QtTestAggregate"_s + identity;
    const QString uid = aggregateUidPrefix() + QString::number(ownerPid) + u'.' + identity;

    QCFType<CFMutableDictionaryRef> description{ CFDictionaryCreateMutable(
            kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks,
            &kCFTypeDictionaryValueCallBacks) };
    QCFType<CFStringRef> cfName = name.toCFString();
    QCFType<CFStringRef> cfUid = uid.toCFString();
    QCFType<CFStringRef> cfSubDeviceUid = subDeviceUid->toCFString();
    const int isPrivate = visibility == Visibility::Private ? 1 : 0;
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
    if (AudioHardwareCreateAggregateDevice(description, &id) != noErr || id == kAudioObjectUnknown)
        return nullptr;

    auto priv = std::make_unique<Private>();
    priv->defaultSelector = defaultDeviceSelector(direction);
    priv->id = id;
    priv->name = name;

    return std::unique_ptr<QCoreAudioAggregateDevice>{
        new QCoreAudioAggregateDevice{ std::move(priv) },
    };
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

bool QCoreAudioAggregateDevice::makeSystemDefault()
{
    if (d->id == kAudioObjectUnknown)
        return false;

    const std::optional<AudioObjectID> previous = defaultDeviceId(d->defaultSelector);
    if (!previous)
        return false;
    if (*previous == d->id)
        return true;

    // CoreAudio returns noErr for a private device but keeps the previous default, so the
    // readback decides
    if (!setDefaultDeviceId(d->defaultSelector, d->id))
        return false;

    QDeadlineTimer deadline{ 1s };
    while (defaultDeviceId(d->defaultSelector) != d->id) {
        if (deadline.hasExpired())
            return false;
        std::this_thread::sleep_for(10ms);
    }

    // Keep the first default, so destruction restores the device that was there before any change
    if (!d->previousDefaultId)
        d->previousDefaultId = previous;
    return true;
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
