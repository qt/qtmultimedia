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

#include "qmediaplatformdevicemanager_p.h"
#include "qaudiodeviceinfo.h"
#include "qcamerainfo.h"
#include "qaudiosystem_p.h"

QT_BEGIN_NAMESPACE

QMediaPlatformDeviceManager::QMediaPlatformDeviceManager()
{
}

QMediaPlatformDeviceManager::~QMediaPlatformDeviceManager()
{
}

QAudioDeviceInfo QMediaPlatformDeviceManager::audioInput(const QByteArray &id) const
{
    const auto inputs = audioInputs();
    for (auto i : inputs) {
        if (i.id() == id)
            return i;
    }
    return {};
}

QAudioDeviceInfo QMediaPlatformDeviceManager::audioOutput(const QByteArray &id) const
{
    const auto outputs = audioOutputs();
    for (auto o : outputs) {
        if (o.id() == id)
            return o;
    }
    return {};
}

QCameraInfo QMediaPlatformDeviceManager::videoInput(const QByteArray &id) const
{
    const auto inputs = videoInputs();
    for (auto i : inputs) {
        if (i.id() == id)
            return i;
    }
    return QCameraInfo();
}

QAbstractAudioInput* QMediaPlatformDeviceManager::audioInputDevice(const QAudioFormat &format, const QAudioDeviceInfo &deviceInfo)
{
    QAudioDeviceInfo info = deviceInfo;
    if (info.isNull())
        info = audioInputs().value(0);

    QAbstractAudioInput* p = createAudioInputDevice(info);
    if (p)
        p->setFormat(format);
    return p;
}

QAbstractAudioOutput* QMediaPlatformDeviceManager::audioOutputDevice(const QAudioFormat &format, const QAudioDeviceInfo &deviceInfo)
{
    QAudioDeviceInfo info = deviceInfo;
    if (info.isNull())
        info = audioOutputs().value(0);

    QAbstractAudioOutput* p = createAudioOutputDevice(info);
    if (p)
        p->setFormat(format);
    return p;
}


QT_END_NAMESPACE
