// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qmacosaudiodatautils_p.h"

QT_BEGIN_NAMESPACE

AudioObjectPropertyAddress
makePropertyAddress(
    AudioObjectPropertySelector selector,
    QAudioDevice::Mode mode,
    AudioObjectPropertyElement element)
{
    return { selector,
             mode == QAudioDevice::Input ? kAudioDevicePropertyScopeInput
                                         : kAudioDevicePropertyScopeOutput,
             element };
}

bool getAudioData(
    AudioObjectID objectID,
    const AudioObjectPropertyAddress &address,
    void *dst,
    UInt32 dstSize,
    const char *logName)
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

QByteArray qCoreAudioReadPersistentAudioDeviceID(
    AudioDeviceID device,
    QAudioDevice::Mode mode)
{
    const AudioObjectPropertyAddress propertyAddress = makePropertyAddress(
        kAudioDevicePropertyDeviceUID,
        mode);

    const std::optional<CFStringRef> name = getAudioObject<CFStringRef>(
        device,
        propertyAddress,
        "Device UID");
    if (name) {
        QString s = QString::fromCFString(*name);
        CFRelease(*name);
        return s.toUtf8();
    }

    return QByteArray();
}

QT_END_NAMESPACE
