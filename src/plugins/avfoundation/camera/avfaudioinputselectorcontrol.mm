/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "avfcameradebug.h"
#include "avfaudioinputselectorcontrol.h"
#include "avfcameraservice.h"

#import <AVFoundation/AVFoundation.h>

QT_USE_NAMESPACE

AVFAudioInputSelectorControl::AVFAudioInputSelectorControl(AVFCameraService *service, QObject *parent)
   : QAudioInputSelectorControl(parent)
   , m_service(service)
   , m_dirty(true)
{
    NSArray *videoDevices = [AVCaptureDevice devicesWithMediaType:AVMediaTypeAudio];
    for (AVCaptureDevice *device in videoDevices) {
        QString deviceId = QString::fromUtf8([[device uniqueID] UTF8String]);
        m_devices << deviceId;
        m_deviceDescriptions.insert(deviceId,
                                    QString::fromUtf8([[device localizedName] UTF8String]));
    }

    m_activeInput = m_devices.first();
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
    return m_devices.first();
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
    AVCaptureDevice *device = 0;

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
