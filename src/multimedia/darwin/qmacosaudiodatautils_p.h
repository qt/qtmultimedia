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

#include "qaudiodevice.h"
#include "qdebug.h"

#include <optional>
#include <vector>
#include <algorithm>

QT_BEGIN_NAMESPACE

template<typename... Args>
void printUnableToReadWarning(const char *logName, AudioObjectID objectID, const AudioObjectPropertyAddress &address, Args &&...args)
{
    if (!logName)
        return;

    char scope[5] = {0};
    memcpy(&scope, &address.mScope, 4);
    std::reverse(scope, scope + 4);

    auto warn = qWarning();
    warn << "Unable to read property" << logName << "for object" << objectID << ", scope" << scope << ";";
    (warn << ... << args);
    warn << "\n  If the warning is unexpected use test_audio_config to get comprehensive audio info and report a bug";
}

inline static AudioObjectPropertyAddress
makePropertyAddress(AudioObjectPropertySelector selector, QAudioDevice::Mode mode,
                    AudioObjectPropertyElement element = kAudioObjectPropertyElementMaster)
{
    return { selector,
             mode == QAudioDevice::Input ? kAudioDevicePropertyScopeInput
                                         : kAudioDevicePropertyScopeOutput,
             element };
}

inline static bool getAudioData(AudioObjectID objectID, const AudioObjectPropertyAddress &address,
                                void *dst, UInt32 dstSize, const char *logName)
{
    UInt32 readBytes = dstSize;
    const auto res = AudioObjectGetPropertyData(objectID, &address, 0, nullptr, &readBytes, dst);

    if (res != noErr)
        printUnableToReadWarning(logName, objectID, address, "Err:", res);
    else if (readBytes != dstSize)
        printUnableToReadWarning(logName, objectID, address, "Data size", readBytes, "VS", dstSize,
                                 "expected");
    else
        return true;

    return false;
}

template<typename T>
std::optional<std::vector<T>> getAudioData(AudioObjectID objectID,
                                           const AudioObjectPropertyAddress &address,
                                           const char *logName, size_t minDataSize = 0)
{
    static_assert(std::is_trivial_v<T>, "A trivial type is expected");

    UInt32 size = 0;
    const auto res = AudioObjectGetPropertyDataSize(objectID, &address, 0, nullptr, &size);

    if (res != noErr) {
        printUnableToReadWarning(logName, objectID, address,
                                 "AudioObjectGetPropertyDataSize failed, Err:", res);
    } else if (size / sizeof(T) < minDataSize) {
        printUnableToReadWarning(logName, objectID, address, "Data size is too small:", size, "VS",
                                 minDataSize * sizeof(T), "bytes");
    } else {
        std::vector<T> data(size / sizeof(T));
        if (getAudioData(objectID, address, data.data(), data.size() * sizeof(T), logName))
            return { std::move(data) };
    }

    return {};
}

template<typename T>
std::optional<T> getAudioObject(AudioObjectID objectID, const AudioObjectPropertyAddress &address,
                                const char *logName)
{
    static_assert(std::is_trivial_v<T>, "A trivial type is expected");

    T object{};
    if (getAudioData(objectID, address, &object, sizeof(T), logName))
        return { object };

    return {};
}

QT_END_NAMESPACE

#endif // QMACOSAUDIODATAUTILS_P_H
