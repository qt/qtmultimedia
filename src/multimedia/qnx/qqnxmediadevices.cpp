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

#include "qqnxmediadevices_p.h"
#include "qmediadevices.h"
#include "private/qcameradevice_p.h"
#include "qcameradevice.h"

#include "qqnxaudiosource_p.h"
#include "qqnxaudiosink_p.h"
#include "qqnxaudiodevice_p.h"

#include <qdir.h>
#include <qdebug.h>

QT_BEGIN_NAMESPACE

static QList<QAudioDevice> enumeratePcmDevices(QAudioDevice::Mode mode)
{
    if (mode == QAudioDevice::Null)
        return {};

    QDir dir(QStringLiteral("/dev/snd"));

    dir.setFilter(QDir::Files);
    dir.setSorting(QDir::Name);

    // QNX PCM devices names start with the pcm prefix and end either with the
    // 'p' (playback) or 'c' (capture) suffix

    const char modeSuffix = mode == QAudioDevice::Input ? 'c' : 'p';

    QList<QAudioDevice> devices;

    for (const QString &entry : dir.entryList()) {
        if (entry.startsWith(QStringLiteral("pcm")) && entry.back() == modeSuffix)
            devices << (new QnxAudioDeviceInfo(entry.toUtf8(), mode))->create();
    }

    return devices;
}

QQnxMediaDevices::QQnxMediaDevices()
    : QPlatformMediaDevices()
{
}

QList<QAudioDevice> QQnxMediaDevices::audioInputs() const
{
    return ::enumeratePcmDevices(QAudioDevice::Input);
}

QList<QAudioDevice> QQnxMediaDevices::audioOutputs() const
{
    return ::enumeratePcmDevices(QAudioDevice::Output);
}

QPlatformAudioSource *QQnxMediaDevices::createAudioSource(const QAudioDevice &deviceInfo)
{
    return new QQnxAudioSource(deviceInfo);
}

QPlatformAudioSink *QQnxMediaDevices::createAudioSink(const QAudioDevice &deviceInfo)
{
    return new QQnxAudioSink(deviceInfo);
}

QT_END_NAMESPACE
