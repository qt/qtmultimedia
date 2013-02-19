/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <qaudiodeviceinfo.h>

#include "qpulseaudioplugin.h"
#include "qaudiodeviceinfo_pulse.h"
#include "qaudiooutput_pulse.h"
#include "qaudioinput_pulse.h"
#include "qpulseaudioengine.h"

QT_BEGIN_NAMESPACE

QPulseAudioPlugin::QPulseAudioPlugin(QObject *parent)
    : QAudioSystemPlugin(parent)
    , m_pulseEngine(QPulseAudioEngine::instance())
{
}

QList<QByteArray> QPulseAudioPlugin::availableDevices(QAudio::Mode mode) const
{
    return m_pulseEngine->availableDevices(mode);
}

QAbstractAudioInput *QPulseAudioPlugin::createInput(const QByteArray &device)
{
    QPulseAudioInput *input = new QPulseAudioInput(device);
    return input;
}

QAbstractAudioOutput *QPulseAudioPlugin::createOutput(const QByteArray &device)
{

    QPulseAudioOutput *output = new QPulseAudioOutput(device);
    return output;
}

QAbstractAudioDeviceInfo *QPulseAudioPlugin::createDeviceInfo(const QByteArray &device, QAudio::Mode mode)
{
    QPulseAudioDeviceInfo *deviceInfo = new QPulseAudioDeviceInfo(device, mode);
    return deviceInfo;
}

QT_END_NAMESPACE
