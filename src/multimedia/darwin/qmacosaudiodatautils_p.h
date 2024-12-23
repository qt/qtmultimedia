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
#include <QtCore/private/qcore_mac_p.h>

#include <algorithm>
#include <optional>
#include <vector>

QT_BEGIN_NAMESPACE

namespace QCoreAudioUtils
{

// coreaudio string helpers
QStringView audioPropertySelectorToString(AudioObjectPropertySelector);
QStringView audioPropertyScopeToString(AudioObjectPropertyScope);
QStringView audioPropertyElementToString(AudioObjectPropertyElement);

} // namespace QCoreAudioUtils


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

[[nodiscard]] bool getAudioData(
    AudioObjectID objectID,
    const AudioObjectPropertyAddress &address,
    void *dst,
    UInt32 dstSize,
    bool warnIfMissing = true);

template<typename T>
std::optional<std::vector<T>> getAudioData(AudioObjectID objectID,
                                           const AudioObjectPropertyAddress &address,
                                           size_t minDataSize = 0,
                                           bool warnIfMissing = true)
{
    static_assert(std::is_trivial_v<T>, "A trivial type is expected");

    UInt32 size = 0;
    const auto res = AudioObjectGetPropertyDataSize(objectID, &address, 0, nullptr, &size);

    if (res != noErr) {
        if (warnIfMissing)
            printUnableToReadWarning(objectID, address,
                                     "AudioObjectGetPropertyDataSize failed, Err:", res);
    } else if (size / sizeof(T) < minDataSize) {
        if (warnIfMissing)
            printUnableToReadWarning(objectID, address, "Data size is too small:", size, "VS",
                                     minDataSize * sizeof(T), "bytes");
    } else {
        std::vector<T> data(size / sizeof(T));
        if (getAudioData(objectID, address, data.data(), data.size() * sizeof(T)))
            return { std::move(data) };
    }

    return {};
}

template<typename T>
std::optional<T> getAudioObject(AudioObjectID objectID, const AudioObjectPropertyAddress &address,
                                bool warnIfMissing = false)
{
    if constexpr(std::is_same_v<T, QCFString>) {
        const std::optional<CFStringRef> string = getAudioObject<CFStringRef>(
                objectID, address, warnIfMissing);
        if (string)
            return QCFString{*string};

        return {};
    } else {
        static_assert(std::is_trivial_v<T>, "A trivial type is expected");

        T object{};
        if (getAudioData(objectID, address, &object, sizeof(T), warnIfMissing))
            return object;

        return {};
    }
}


[[nodiscard]] QByteArray qCoreAudioReadPersistentAudioDeviceID(
    AudioDeviceID device,
    QAudioDevice::Mode mode);

[[nodiscard]] std::optional<AudioDeviceID> qCoreAudioFindAudioDeviceId(
    const QByteArray &id,
    QAudioDevice::Mode mode);

[[nodiscard]] std::optional<AudioDeviceID> qCoreAudioFindAudioDeviceId(
    const QAudioDevice &device);

QT_END_NAMESPACE

#endif // QMACOSAUDIODATAUTILS_P_H
