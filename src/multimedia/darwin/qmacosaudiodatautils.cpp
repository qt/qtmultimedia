// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qmacosaudiodatautils_p.h"

QT_BEGIN_NAMESPACE

namespace QCoreAudioUtils
{

#define ENUM_CASE(NAME) \
case NAME: return QStringView(QT_UNICODE_LITERAL(QT_STRINGIFY(NAME)))

#define DUPLICATE_ENUM_CASE(NAME) static_assert(true, "force semicolon")

QStringView audioPropertySelectorToString(AudioObjectPropertySelector selector)
{
    switch (selector) {
        // AudioObject properties
        ENUM_CASE(kAudioObjectPropertyBaseClass          );
        ENUM_CASE(kAudioObjectPropertyClass              );
        ENUM_CASE(kAudioObjectPropertyOwner              );
        ENUM_CASE(kAudioObjectPropertyName               );
        ENUM_CASE(kAudioObjectPropertyModelName          );
        ENUM_CASE(kAudioObjectPropertyManufacturer       );
        ENUM_CASE(kAudioObjectPropertyElementName        );
        ENUM_CASE(kAudioObjectPropertyElementCategoryName);
        ENUM_CASE(kAudioObjectPropertyElementNumberName  );
        ENUM_CASE(kAudioObjectPropertyOwnedObjects       );
        ENUM_CASE(kAudioObjectPropertyIdentify           );
        ENUM_CASE(kAudioObjectPropertySerialNumber       );
        ENUM_CASE(kAudioObjectPropertyFirmwareVersion    );
        ENUM_CASE(kAudioObjectPropertySelectorWildcard   );

               // AudioDevice properties
        ENUM_CASE(kAudioDevicePropertyConfigurationApplication      );
        ENUM_CASE(kAudioDevicePropertyDeviceUID                     );
        ENUM_CASE(kAudioDevicePropertyModelUID                      );
        ENUM_CASE(kAudioDevicePropertyTransportType                 );
        ENUM_CASE(kAudioDevicePropertyRelatedDevices                );
        ENUM_CASE(kAudioDevicePropertyClockDomain                   );
        ENUM_CASE(kAudioDevicePropertyDeviceIsAlive                 );
        ENUM_CASE(kAudioDevicePropertyDeviceIsRunning               );
        ENUM_CASE(kAudioDevicePropertyDeviceCanBeDefaultDevice      );
        ENUM_CASE(kAudioDevicePropertyDeviceCanBeDefaultSystemDevice);
        ENUM_CASE(kAudioDevicePropertyLatency                       );
        ENUM_CASE(kAudioDevicePropertyStreams                       );
        ENUM_CASE(kAudioObjectPropertyControlList                   );
        ENUM_CASE(kAudioDevicePropertySafetyOffset                  );
        ENUM_CASE(kAudioDevicePropertyNominalSampleRate             );
        ENUM_CASE(kAudioDevicePropertyAvailableNominalSampleRates   );
        ENUM_CASE(kAudioDevicePropertyIcon                          );
        ENUM_CASE(kAudioDevicePropertyIsHidden                      );
        ENUM_CASE(kAudioDevicePropertyPreferredChannelsForStereo    );
        ENUM_CASE(kAudioDevicePropertyPreferredChannelLayout        );

               // AudioClockDevice properties
        ENUM_CASE(kAudioClockDevicePropertyDeviceUID                            );
        DUPLICATE_ENUM_CASE(kAudioClockDevicePropertyTransportType              );
        DUPLICATE_ENUM_CASE(kAudioClockDevicePropertyClockDomain                );
        DUPLICATE_ENUM_CASE(kAudioClockDevicePropertyDeviceIsAlive              );
        DUPLICATE_ENUM_CASE(kAudioClockDevicePropertyDeviceIsRunning            );
        DUPLICATE_ENUM_CASE(kAudioClockDevicePropertyLatency                    );
        DUPLICATE_ENUM_CASE(kAudioClockDevicePropertyControlList                );
        DUPLICATE_ENUM_CASE(kAudioClockDevicePropertyNominalSampleRate          );
        DUPLICATE_ENUM_CASE(kAudioClockDevicePropertyAvailableNominalSampleRates);

               // AudioEndPointDevice properties
        ENUM_CASE(kAudioEndPointDevicePropertyComposition );
        ENUM_CASE(kAudioEndPointDevicePropertyEndPointList);
        ENUM_CASE(kAudioEndPointDevicePropertyIsPrivate   );

               // AudioStream properties
        ENUM_CASE(kAudioStreamPropertyIsActive                );
        ENUM_CASE(kAudioStreamPropertyDirection               );
        ENUM_CASE(kAudioStreamPropertyTerminalType            );
        ENUM_CASE(kAudioStreamPropertyStartingChannel         );
        ENUM_CASE(kAudioStreamPropertyVirtualFormat           );
        ENUM_CASE(kAudioStreamPropertyAvailableVirtualFormats );
        ENUM_CASE(kAudioStreamPropertyPhysicalFormat          );
        ENUM_CASE(kAudioStreamPropertyAvailablePhysicalFormats);

    default:
        Q_UNREACHABLE_RETURN(u"");
    }
}

QStringView audioPropertyScopeToString(AudioObjectPropertyScope scope)
{
    switch (scope) {
        ENUM_CASE(kAudioObjectPropertyScopeGlobal     );
        ENUM_CASE(kAudioObjectPropertyScopeInput      );
        ENUM_CASE(kAudioObjectPropertyScopeOutput     );
        ENUM_CASE(kAudioObjectPropertyScopePlayThrough);
        ENUM_CASE(kAudioObjectPropertyScopeWildcard   );
    default:
        Q_UNREACHABLE_RETURN(u"");
    }
}

QStringView audioPropertyElementToString(AudioObjectPropertyElement element)
{
    switch (element) {
        ENUM_CASE(kAudioObjectPropertyElementMain    );
        ENUM_CASE(kAudioObjectPropertyElementWildcard);
    default:
        Q_UNREACHABLE_RETURN(u"");
    }
}

#undef ENUM_CASE
#undef DUPLICATE_ENUM_CASE

}

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

bool getAudioData(AudioObjectID objectID, const AudioObjectPropertyAddress &address, void *dst,
                  UInt32 dstSize, bool warnIfMissing)
{
    UInt32 readBytes = dstSize;
    const auto res = AudioObjectGetPropertyData(objectID, &address, 0, nullptr, &readBytes, dst);

    if (res != noErr) {
        if (warnIfMissing)
            printUnableToReadWarning(objectID, address, "Err:", res);
    } else if (readBytes != dstSize) {
        if (warnIfMissing)
            printUnableToReadWarning(objectID, address, "Data size", readBytes, "VS", dstSize,
                                     "expected");
    } else {
        return true;
    }

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
        propertyAddress);
    if (name) {
        QString s = QString::fromCFString(*name);
        CFRelease(*name);
        return s.toUtf8();
    }

    return QByteArray();
}

QT_END_NAMESPACE
