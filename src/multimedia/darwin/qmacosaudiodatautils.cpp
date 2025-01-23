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
QCoreAudioUtils::makePropertyAddress(
    AudioObjectPropertySelector selector,
    QAudioDevice::Mode mode,
    AudioObjectPropertyElement element)
{
    return { selector,
             mode == QAudioDevice::Input ? kAudioDevicePropertyScopeInput
                                         : kAudioDevicePropertyScopeOutput,
             element };
}

bool QCoreAudioUtils::getAudioPropertyRaw(AudioObjectID objectID, const AudioObjectPropertyAddress &address,
                         QSpan<std::byte> destination, bool warnIfMissing)
{
    UInt32 readBytes = destination.size();
    const auto res =
            AudioObjectGetPropertyData(objectID, &address, 0, nullptr, &readBytes, destination.data());

    if (res != noErr) {
        if (warnIfMissing)
            printUnableToReadWarning(objectID, address, "Err:", res);
    } else if (readBytes != destination.size()) {
        if (warnIfMissing)
            printUnableToReadWarning(objectID, address, "Data size", readBytes, "VS", destination.size(),
                                     "expected");
    } else {
        return true;
    }

    return false;
}

QByteArray QCoreAudioUtils::readPersistentDeviceId(
    AudioDeviceID device,
    QAudioDevice::Mode mode)
{
    const AudioObjectPropertyAddress propertyAddress = makePropertyAddress(
        kAudioDevicePropertyDeviceUID,
        mode);

    const std::optional<QCFString> name = getAudioProperty<QCFString>(device, propertyAddress);
    if (name)
        return QString{*name}.toUtf8();

    return QByteArray();
}

std::optional<AudioDeviceID> QCoreAudioUtils::findAudioDeviceId(
    const QByteArray &id,
    QAudioDevice::Mode mode)
{
    if (id.isEmpty() || mode == QAudioDevice::Mode::Null)
        return std::nullopt;
    // Iterate over all the devices connected and find the one matching our ID and return the AudioDeviceID.
    const AudioObjectPropertyAddress audioDevicesPropertyAddress = {
        kAudioHardwarePropertyDevices, kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMain
    };
    const std::optional<std::vector<AudioDeviceID>> audioDevicesOpt = getAudioPropertyList<AudioDeviceID>(
        kAudioObjectSystemObject,
        audioDevicesPropertyAddress);
    if (audioDevicesOpt.has_value()) {
        const std::vector<AudioDeviceID> &audioDevices = audioDevicesOpt.value();
        const AudioObjectPropertyAddress audioDeviceStreamFormatPropertyAddress = makePropertyAddress(
            kAudioDevicePropertyStreamFormat,
            mode);
        for (const AudioDeviceID &device : audioDevices) {
            // Ignore devices that don't have any audio formats we can use.
            const std::optional<AudioStreamBasicDescription> audioStreamOpt =
                getAudioProperty<AudioStreamBasicDescription>(
                    device,
                    audioDeviceStreamFormatPropertyAddress);
            // Check that these devices have the same unique-id. In which case, we found the
            // correct CoreAudioID.
            if (audioStreamOpt.has_value() && readPersistentDeviceId(device, mode) == id)
                return device;
        }
    }
    return std::nullopt;
}

std::optional<AudioDeviceID> QCoreAudioUtils::findAudioDeviceId(
    const QAudioDevice &device)
{
    return findAudioDeviceId(device.id(), device.mode());
}

QT_END_NAMESPACE
