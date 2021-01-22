/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd and/or its subsidiary(-ies).
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#include "avfcameradebug.h"
#include "avfaudioinputselectorcontrol.h"
#include "avfcameraservice.h"

#import <AVFoundation/AVFoundation.h>

QT_USE_NAMESPACE

AVFAudioInputSelectorControl::AVFAudioInputSelectorControl(AVFCameraService *service, QObject *parent)
   : QAudioInputSelectorControl(parent)
   , m_dirty(true)
{
    Q_UNUSED(service);
    NSArray *videoDevices = [AVCaptureDevice devicesWithMediaType:AVMediaTypeAudio];
    for (AVCaptureDevice *device in videoDevices) {
        QString deviceId = QString::fromUtf8([[device uniqueID] UTF8String]);
        m_devices << deviceId;
        m_deviceDescriptions.insert(deviceId,
                                    QString::fromUtf8([[device localizedName] UTF8String]));
    }

    AVCaptureDevice *defaultDevice = [AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeAudio];
    if (defaultDevice) {
        m_defaultDevice = QString::fromUtf8([defaultDevice.uniqueID UTF8String]);
        m_activeInput = m_defaultDevice;
    }
}

AVFAudioInputSelectorControl::~AVFAudioInputSelectorControl()
{
}

QList<QString> AVFAudioInputSelectorControl::availableInputs() const
{
    return m_devices;
}

QString AVFAudioInputSelectorControl::inputDescription(const QString &name) const
{
    return m_deviceDescriptions.value(name);
}

QString AVFAudioInputSelectorControl::defaultInput() const
{
    return m_defaultDevice;
}

QString AVFAudioInputSelectorControl::activeInput() const
{
    return m_activeInput;
}

void AVFAudioInputSelectorControl::setActiveInput(const QString &name)
{
    if (name != m_activeInput) {
        m_activeInput = name;
        m_dirty = true;

        Q_EMIT activeInputChanged(m_activeInput);
    }
}

AVCaptureDevice *AVFAudioInputSelectorControl::createCaptureDevice()
{
    m_dirty = false;
    AVCaptureDevice *device = nullptr;

    if (!m_activeInput.isEmpty()) {
        device = [AVCaptureDevice deviceWithUniqueID:
                    [NSString stringWithUTF8String:
                        m_activeInput.toUtf8().constData()]];
    }

    if (!device)
        device = [AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeAudio];

    return device;
}

#include "moc_avfaudioinputselectorcontrol.cpp"
