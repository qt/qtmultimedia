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

#include "qalsamediadevices_p.h"
#include "qmediadevices.h"
#include "qcameradevice_p.h"

#include "private/qalsaaudiosource_p.h"
#include "private/qalsaaudiosink_p.h"
#include "private/qalsaaudiodevice_p.h"

#include <alsa/asoundlib.h>

QT_BEGIN_NAMESPACE

QAlsaMediaDevices::QAlsaMediaDevices()
    : QPlatformMediaDevices()
{
}

static QList<QAudioDevice> availableDevices(QAudioDevice::Mode mode)
{
    QList<QAudioDevice> devices;

    QByteArray filter;

    // Create a list of all current audio devices that support mode
    void **hints, **n;
    char *name, *descr, *io;

    if(snd_device_name_hint(-1, "pcm", &hints) < 0) {
        qWarning() << "no alsa devices available";
        return devices;
    }
    n = hints;

    if(mode == QAudioDevice::Input) {
        filter = "Input";
    } else {
        filter = "Output";
    }

    while (*n != NULL) {
        name = snd_device_name_get_hint(*n, "NAME");
        if (name != 0 && qstrcmp(name, "null") != 0) {
            descr = snd_device_name_get_hint(*n, "DESC");
            io = snd_device_name_get_hint(*n, "IOID");

            if ((descr != NULL) && ((io == NULL) || (io == filter))) {
                auto *infop = new QAlsaAudioDeviceInfo(name, QString::fromUtf8(descr), mode);
                devices.append(infop->create());
                if (strcmp(name, "default") == 0)
                    infop->isDefault = true;
            }

            free(descr);
            free(io);
        }
        free(name);
        ++n;
    }
    snd_device_name_free_hint(hints);

    return devices;
}

QList<QAudioDevice> QAlsaMediaDevices::audioInputs() const
{
    return availableDevices(QAudioDevice::Input);
}

QList<QAudioDevice> QAlsaMediaDevices::audioOutputs() const
{
    return availableDevices(QAudioDevice::Output);
}

QList<QCameraDevice> QAlsaMediaDevices::videoInputs() const
{
    return {};
}

QPlatformAudioSource *QAlsaMediaDevices::createAudioSource(const QAudioDevice &deviceInfo)
{
    return new QAlsaAudioSource(deviceInfo.id());
}

QPlatformAudioSink *QAlsaMediaDevices::createAudioSink(const QAudioDevice &deviceInfo)
{
    return new QAlsaAudioSink(deviceInfo.id());
}

QT_END_NAMESPACE
