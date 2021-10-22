/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qdarwinmediadevices_p.h"
#include "qmediadevices.h"
#include "qcameradevice_p.h"
#include "qaudiodevice_p.h"
#include "private/qdarwinaudiodevice_p.h"
#include "private/qdarwinaudiosource_p.h"
#include "private/qdarwinaudiosink_p.h"
#include "private/avfcamera_p.h"
#include "private/avfcamerautility_p.h"
#include "private/avfvideobuffer_p.h"

#include <CoreVideo/CoreVideo.h>
#import <AVFoundation/AVFoundation.h>

#if defined(Q_OS_IOS) || defined(Q_OS_TVOS)
#include "private/qcoreaudiosessionmanager_p.h"
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
    if (defaultDevice != 0)
        devices << (new QCoreAudioDeviceInfo(defaultDevice, uniqueId(defaultDevice, mode), mode))->create();

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
    NSNotificationCenter *notificationCenter = [NSNotificationCenter defaultCenter];
    m_deviceConnectedObserver = [notificationCenter addObserverForName:AVCaptureDeviceWasConnectedNotification
                                                                object:nil
                                                                queue:[NSOperationQueue mainQueue]
                                                                usingBlock:^(NSNotification *) {
                                                                        this->updateCameraDevices();
            this->updateAudioDevices();
                                                                }];

    m_deviceDisconnectedObserver = [notificationCenter addObserverForName:AVCaptureDeviceWasDisconnectedNotification
                                                                object:nil
                                                                queue:[NSOperationQueue mainQueue]
                                                                usingBlock:^(NSNotification *) {
                                                                        this->updateCameraDevices();
            this->updateAudioDevices();
                                                                }];

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
    updateCameraDevices();
    updateAudioDevices();
}


QDarwinMediaDevices::~QDarwinMediaDevices()
{
    NSNotificationCenter* notificationCenter = [NSNotificationCenter defaultCenter];
    [notificationCenter removeObserver:(id)m_deviceConnectedObserver];
    [notificationCenter removeObserver:(id)m_deviceDisconnectedObserver];

#ifdef Q_OS_MACOS
    AudioObjectRemovePropertyListener(kAudioObjectSystemObject, (AudioObjectPropertyAddress *)m_audioDevicesProperty, audioDeviceChangeListener, this);
#endif
}

QList<QAudioDevice> QDarwinMediaDevices::audioInputs() const
{
#ifdef Q_OS_IOS
    // TODO: Support Bluetooth and USB devices
    QList<QAudioDevice> devices;
    AVCaptureDeviceDiscoverySession *captureDeviceDiscoverySession = [AVCaptureDeviceDiscoverySession
            discoverySessionWithDeviceTypes:@[AVCaptureDeviceTypeBuiltInMicrophone]
            mediaType:AVMediaTypeAudio
            position:AVCaptureDevicePositionUnspecified];

    NSArray *captureDevices = [captureDeviceDiscoverySession devices];
    for (AVCaptureDevice *device in captureDevices)
        devices << (new QCoreAudioDeviceInfo(QString::fromNSString(device.uniqueID).toUtf8(), QAudioDevice::Input))->create();
    return devices;
#else
    return availableAudioDevices(QAudioDevice::Input);
#endif
}

QList<QAudioDevice> QDarwinMediaDevices::audioOutputs() const
{
#ifdef Q_OS_IOS
    QList<QAudioDevice> devices;
    devices.append((new QCoreAudioDeviceInfo("default", QAudioDevice::Output))->create());
    return devices;
#else
    return availableAudioDevices(QAudioDevice::Output);
#endif
}

QList<QCameraDevice> QDarwinMediaDevices::videoInputs() const
{
    return m_cameraDevices;
}

void QDarwinMediaDevices::updateCameraDevices()
{
#ifdef Q_OS_IOS
    // Cameras can't change dynamically on iOS. Update only once.
    if (!m_cameraDevices.isEmpty())
        return;
#endif

    QList<QCameraDevice> cameras;

    AVCaptureDevice *defaultDevice = [AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeVideo];
    NSArray *videoDevices = [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo];

    for (AVCaptureDevice *device in videoDevices) {

        QCameraDevicePrivate *info = new QCameraDevicePrivate;
        if (defaultDevice && [defaultDevice.uniqueID isEqualToString:device.uniqueID])
            info->isDefault = true;
        info->id = QByteArray([[device uniqueID] UTF8String]);
        info->description = QString::fromNSString([device localizedName]);

        QSet<QSize> photoResolutions;
        QList<QCameraFormat> videoFormats;

        for (AVCaptureDeviceFormat *format in device.formats) {
            if (![format.mediaType isEqualToString:AVMediaTypeVideo])
                continue;

            auto dimensions = CMVideoFormatDescriptionGetDimensions(format.formatDescription);
            QSize resolution(dimensions.width, dimensions.height);
            photoResolutions.insert(resolution);

            float maxFrameRate = 0;
            float minFrameRate = 1.e6;

            auto encoding = CMVideoFormatDescriptionGetCodecType(format.formatDescription);
            auto pixelFormat = AVFVideoBuffer::fromCVPixelFormat(encoding);
            // Ignore pixel formats we can't handle
            if (pixelFormat == QVideoFrameFormat::Format_Invalid)
                continue;

            for (AVFrameRateRange *frameRateRange in format.videoSupportedFrameRateRanges) {
                if (frameRateRange.minFrameRate < minFrameRate)
                    minFrameRate = frameRateRange.minFrameRate;
                if (frameRateRange.maxFrameRate > maxFrameRate)
                    maxFrameRate = frameRateRange.maxFrameRate;
            }

#ifdef Q_OS_IOS
            // From Apple's docs (iOS):
            // By default, AVCaptureStillImageOutput emits images with the same dimensions as
            // its source AVCaptureDevice instance’s activeFormat.formatDescription. However,
            // if you set this property to YES, the receiver emits still images at the capture
            // device’s highResolutionStillImageDimensions value.
            const QSize hrRes(qt_device_format_high_resolution(format));
            if (!hrRes.isNull() && hrRes.isValid())
                photoResolutions.insert(hrRes);
#endif

            auto *f = new QCameraFormatPrivate{
                QSharedData(),
                pixelFormat,
                resolution,
                minFrameRate,
                maxFrameRate
            };
            videoFormats << f->create();
        }
        if (videoFormats.isEmpty()) {
            // skip broken cameras without valid formats
            delete info;
            continue;
        }
        info->videoFormats = videoFormats;
        info->photoResolutions = photoResolutions.values();

        cameras.append(info->create());
    }

    if (cameras != m_cameraDevices) {
        m_cameraDevices = cameras;
        videoInputsChanged();
    }
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
