// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qdarwinmediadevices_p.h"
#include "qmediadevices.h"
#include "private/qaudiodevice_p.h"
#include "qdarwinaudiodevice_p.h"
#include "qdarwinaudiosource_p.h"
#include "qdarwinaudiosink_p.h"

#include <qdebug.h>

#if defined(Q_OS_IOS) || defined(Q_OS_TVOS)
#include "qcoreaudiosessionmanager_p.h"
#import <AVFoundation/AVFoundation.h>
#endif

QT_BEGIN_NAMESPACE

#if defined(Q_OS_MACOS)
AudioDeviceID defaultAudioDevice(QAudioDevice::Mode mode)
{
    AudioDeviceID audioDevice;
    UInt32 size = sizeof(audioDevice);
    const AudioObjectPropertySelector selector = (mode == QAudioDevice::Output) ? kAudioHardwarePropertyDefaultOutputDevice
                                                                               : kAudioHardwarePropertyDefaultInputDevice;
    AudioObjectPropertyAddress defaultDevicePropertyAddress = { selector,
                                                                kAudioObjectPropertyScopeGlobal,
                                                                kAudioObjectPropertyElementMaster };

    if (AudioObjectGetPropertyData(kAudioObjectSystemObject,
                                   &defaultDevicePropertyAddress,
                                   0, NULL, &size, &audioDevice) != noErr) {
        qWarning("QAudioDevice: Unable to find default %s device",  (mode == QAudioDevice::Output) ? "output" : "input");
        return 0;
    }

    return audioDevice;
}

static QByteArray uniqueId(AudioDeviceID device, QAudioDevice::Mode mode)
{
    CFStringRef name;
    UInt32 size = sizeof(CFStringRef);

    AudioObjectPropertyScope audioPropertyScope = mode == QAudioDevice::Input ? kAudioDevicePropertyScopeInput : kAudioDevicePropertyScopeOutput;

    AudioObjectPropertyAddress audioDeviceNamePropertyAddress = { kAudioDevicePropertyDeviceUID,
                                                                  audioPropertyScope,
                                                                  kAudioObjectPropertyElementMaster };

    if (AudioObjectGetPropertyData(device, &audioDeviceNamePropertyAddress, 0, NULL, &size, &name) != noErr) {
        qWarning() << "QAudioDevice: Unable to get device UID";
        return QByteArray();
    }

    QString s = QString::fromCFString(name);
    CFRelease(name);
    return s.toUtf8();
}

QList<QAudioDevice> availableAudioDevices(QAudioDevice::Mode mode)
{

    QList<QAudioDevice> devices;

    AudioDeviceID defaultDevice = defaultAudioDevice(mode);
    if (defaultDevice != 0) {
        auto *dev = new QCoreAudioDeviceInfo(defaultDevice, uniqueId(defaultDevice, mode), mode);
        dev->isDefault = true;
        devices << dev->create();
    }

    UInt32 propSize = 0;
    AudioObjectPropertyAddress audioDevicesPropertyAddress = { kAudioHardwarePropertyDevices,
                                                               kAudioObjectPropertyScopeGlobal,
                                                               kAudioObjectPropertyElementMaster };

    if (AudioObjectGetPropertyDataSize(kAudioObjectSystemObject,
                                       &audioDevicesPropertyAddress,
                                       0, NULL, &propSize) == noErr) {

        const int dc = propSize / sizeof(AudioDeviceID);

        if (dc > 0) {
            AudioDeviceID*  audioDevices = new AudioDeviceID[dc];

            if (AudioObjectGetPropertyData(kAudioObjectSystemObject, &audioDevicesPropertyAddress, 0, NULL, &propSize, audioDevices) == noErr) {
                for (int i = 0; i < dc; ++i) {
                    if (audioDevices[i] == defaultDevice)
                        continue;

                    AudioStreamBasicDescription sf;
                    UInt32 size = sizeof(AudioStreamBasicDescription);
                    AudioObjectPropertyAddress audioDeviceStreamFormatPropertyAddress = { kAudioDevicePropertyStreamFormat,
                                                                                    (mode == QAudioDevice::Input ? kAudioDevicePropertyScopeInput : kAudioDevicePropertyScopeOutput),
                                                                                    kAudioObjectPropertyElementMaster };

                    if (AudioObjectGetPropertyData(audioDevices[i], &audioDeviceStreamFormatPropertyAddress, 0, NULL, &size, &sf) == noErr)
                        devices << (new QCoreAudioDeviceInfo(audioDevices[i], uniqueId(audioDevices[i], mode), mode))->create();
                }
            }

            delete[] audioDevices;
        }
    }

    return devices;
}

static OSStatus
audioDeviceChangeListener(AudioObjectID, UInt32, const AudioObjectPropertyAddress*, void* ptr)
{
    QDarwinMediaDevices *m = static_cast<QDarwinMediaDevices *>(ptr);
    m->updateAudioDevices();
    return 0;
}
#endif


QDarwinMediaDevices::QDarwinMediaDevices()
    : QPlatformMediaDevices()
{

#ifdef Q_OS_MACOS
    OSStatus err = noErr;
    AudioObjectPropertyAddress *audioDevicesAddress = new AudioObjectPropertyAddress{ kAudioHardwarePropertyDevices, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMaster };
    m_audioDevicesProperty = audioDevicesAddress;
    err = AudioObjectAddPropertyListener(kAudioObjectSystemObject, audioDevicesAddress, audioDeviceChangeListener, this);
    if (err)
        qDebug("error on AudioObjectAddPropertyListener");
#else
    // ### This should use the audio session manager
#endif
    updateAudioDevices();
}


QDarwinMediaDevices::~QDarwinMediaDevices()
{

#ifdef Q_OS_MACOS
    AudioObjectRemovePropertyListener(kAudioObjectSystemObject, (AudioObjectPropertyAddress *)m_audioDevicesProperty, audioDeviceChangeListener, this);
#endif
}

QList<QAudioDevice> QDarwinMediaDevices::audioInputs() const
{
#ifdef Q_OS_IOS
    AVCaptureDevice *defaultDevice = [AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeAudio];

    // TODO: Support Bluetooth and USB devices
    QList<QAudioDevice> devices;
    AVCaptureDeviceDiscoverySession *captureDeviceDiscoverySession = [AVCaptureDeviceDiscoverySession
            discoverySessionWithDeviceTypes:@[AVCaptureDeviceTypeBuiltInMicrophone]
            mediaType:AVMediaTypeAudio
            position:AVCaptureDevicePositionUnspecified];

    NSArray *captureDevices = [captureDeviceDiscoverySession devices];
    for (AVCaptureDevice *device in captureDevices) {
        auto *dev = new QCoreAudioDeviceInfo(QString::fromNSString(device.uniqueID).toUtf8(), QAudioDevice::Input);
        if (defaultDevice && [defaultDevice.uniqueID isEqualToString:device.uniqueID])
            dev->isDefault = true;
        devices << dev->create();
    }
    return devices;
#else
    return availableAudioDevices(QAudioDevice::Input);
#endif
}

QList<QAudioDevice> QDarwinMediaDevices::audioOutputs() const
{
#ifdef Q_OS_IOS
    QList<QAudioDevice> devices;
    auto *dev = new QCoreAudioDeviceInfo("default", QAudioDevice::Output);
    dev->isDefault = true;
    devices.append(dev->create());
    return devices;
#else
    return availableAudioDevices(QAudioDevice::Output);
#endif
}

void QDarwinMediaDevices::updateAudioDevices()
{
#ifdef Q_OS_MACOS
    QList<QAudioDevice> inputs = availableAudioDevices(QAudioDevice::Input);
    if (m_audioInputs != inputs) {
        m_audioInputs = inputs;
        audioInputsChanged();
    }

    QList<QAudioDevice> outputs = availableAudioDevices(QAudioDevice::Output);
    if (m_audioOutputs!= outputs) {
        m_audioOutputs = outputs;
        audioOutputsChanged();
    }
#endif
}

QPlatformAudioSource *QDarwinMediaDevices::createAudioSource(const QAudioDevice &info)
{
    return new QDarwinAudioSource(info);
}

QPlatformAudioSink *QDarwinMediaDevices::createAudioSink(const QAudioDevice &info)
{
    return new QDarwinAudioSink(info);
}


QT_END_NAMESPACE
