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

#include "qwasmaudiodevice_p.h"
#include <emscripten.h>
#include <AL/al.h>
#include <AL/alc.h>

QT_BEGIN_NAMESPACE

QWasmAudioDevice::QWasmAudioDevice(const char *device, const char *desc, bool isDef, QAudioDevice::Mode mode)
    : QAudioDevicePrivate(device, mode)
{
    description = QString::fromUtf8(desc);
    isDefault = isDef;

    minimumChannelCount = 1;
    maximumChannelCount = 2;
    minimumSampleRate = 1;
    maximumSampleRate = 192'000;

    // native openAL formats
    supportedSampleFormats.append(QAudioFormat::UInt8);
    supportedSampleFormats.append(QAudioFormat::Int16);

    // Browsers use 32bit floats as native, but emscripten reccomends checking for the exension.
    if (alIsExtensionPresent("AL_EXT_float32"))
        supportedSampleFormats.append(QAudioFormat::Float);

    preferredFormat.setChannelCount(2);

    preferredFormat.setSampleRate(EM_ASM_INT({
        var AudioContext = window.AudioContext || window.webkitAudioContext;
        var ctx = new AudioContext();
        var sr = ctx.sampleRate;
        ctx.close();
        return sr;
    }));

    auto f = QAudioFormat::Float;

    if (!supportedSampleFormats.contains(f))
        f = QAudioFormat::Int16;
    preferredFormat.setSampleFormat(f);
}

QT_END_NAMESPACE
