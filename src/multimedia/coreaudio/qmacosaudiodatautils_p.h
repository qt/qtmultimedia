// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QMACOSAUDIODATAUTILS_P_H
#define QMACOSAUDIODATAUTILS_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <CoreAudio/AudioHardware.h>

#include <QtMultimedia/qaudiodevice.h>
#include <QtMultimedia/private/qcoreaudioutils_p.h>
#include <QtCore/qdebug.h>
#include <QtCore/qspan.h>
#include <QtCore/private/qcore_mac_p.h>

#include <optional>
#include <vector>

QT_BEGIN_NAMESPACE

namespace QCoreAudioUtils
{

// coreaudio string helpers
QStringView audioPropertySelectorToString(AudioObjectPropertySelector);
QStringView audioPropertyScopeToString(AudioObjectPropertyScope);
QStringView audioPropertyElementToString(AudioObjectPropertyElement);

[[nodiscard]] QByteArray readPersistentDeviceId(
    AudioDeviceID,
    QAudioDevice::Mode);

[[nodiscard]] std::optional<AudioDeviceID> findAudioDeviceId(
    const QByteArray &id,
    QAudioDevice::Mode);

[[nodiscard]] std::optional<AudioDeviceID> findAudioDeviceId(const QAudioDevice &device);

template<typename... Args>
void printUnableToReadWarning(AudioObjectID objectID, const AudioObjectPropertyAddress &address, Args &&...args)
{
    using namespace QCoreAudioUtils;
    auto warn = qWarning();
    warn << "Unable to read property" << QCoreAudioUtils::audioPropertySelectorToString(address.mSelector)
         << "for object" << objectID << ", scope" << QCoreAudioUtils::audioPropertyScopeToString(address.mScope) << ";";
    (warn << ... << args);
    warn << "\n  If the warning is unexpected use test_audio_config to get comprehensive audio info and report a bug";
}


[[nodiscard]] AudioObjectPropertyAddress makePropertyAddress(
    AudioObjectPropertySelector selector,
    QAudioDevice::Mode mode,
    AudioObjectPropertyElement element = kAudioObjectPropertyElementMain);

[[nodiscard]] bool getAudioPropertyRaw(
    AudioObjectID objectID,
    const AudioObjectPropertyAddress &address,
    QSpan<std::byte> destination,
    bool warnIfMissing = true);

template<typename T>
std::optional<std::vector<T>> getAudioPropertyList(AudioObjectID objectID,
                                                   const AudioObjectPropertyAddress &address,
                                                   bool warnIfMissing = true)
{
    static_assert(std::is_trivial_v<T>, "A trivial type is expected");

    UInt32 size = 0;
    const auto res = AudioObjectGetPropertyDataSize(objectID, &address, 0, nullptr, &size);

    if (res != noErr) {
        if (warnIfMissing)
            printUnableToReadWarning(objectID, address,
                                     "AudioObjectGetPropertyDataSize failed, Err:", res);
    } else {
        std::vector<T> data(size / sizeof(T));
        if (getAudioPropertyRaw(objectID, address, as_writable_bytes(QSpan{data})))
            return data;
    }

    return {};
}

template<typename T>
std::optional<T> getAudioProperty(AudioObjectID objectID, const AudioObjectPropertyAddress &address,
                                  bool warnIfMissing = false)
{
    if constexpr(std::is_same_v<T, QCFString>) {
        const std::optional<CFStringRef> string = getAudioProperty<CFStringRef>(
                objectID, address, warnIfMissing);
        if (string)
            return QCFString{*string};

        return {};
    } else {
        static_assert(std::is_trivial_v<T>, "A trivial type is expected");

        T object{};
        if (getAudioPropertyRaw(objectID, address, as_writable_bytes(QSpan(&object, 1)), warnIfMissing))
            return object;

        return {};
    }
}

template<typename T>
std::unique_ptr<T, QCoreAudioUtils::QFreeDeleter>
getAudioPropertyWithFlexibleArrayMember(AudioObjectID objectID, const AudioObjectPropertyAddress &address,
                                        bool warnIfMissing = false)
{
    static_assert(std::is_trivial_v<T>, "A trivial type is expected");

    UInt32 size = 0;
    const auto res = AudioObjectGetPropertyDataSize(objectID, &address, 0, nullptr, &size);
    if (res != noErr) {
        if (warnIfMissing)
            printUnableToReadWarning(objectID, address,
                                     "AudioObjectGetPropertyDataSize failed, Err:", res);
        return nullptr;
    }
    if (size < sizeof(T)) {
        printUnableToReadWarning(objectID, address, "Data size is too small:", size, "VS",
                                 sizeof(T), "bytes");
        return nullptr;
    }

    using QCoreAudioUtils::QFreeDeleter;
    std::unique_ptr<std::byte, QFreeDeleter> region {
        reinterpret_cast<std::byte*>(::malloc(size))
    };

    if (getAudioPropertyRaw(objectID, address,
                            QSpan(region.get(), size), warnIfMissing))
        return std::unique_ptr<T, QFreeDeleter>{
            reinterpret_cast<T*>(region.release())
        };

    return {};
}

} // namespace QCoreAudioUtils



QT_END_NAMESPACE

#endif // QMACOSAUDIODATAUTILS_P_H
