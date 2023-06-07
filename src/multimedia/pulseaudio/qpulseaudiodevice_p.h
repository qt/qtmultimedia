// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QAUDIODEVICEINFOPULSE_H
#define QAUDIODEVICEINFOPULSE_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qbytearray.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qlist.h>

#include "qaudio.h"
#include "qaudiodevice.h"
#include <private/qaudiosystem_p.h>
#include <private/qaudiodevice_p.h>

#include <pulse/pulseaudio.h>

QT_BEGIN_NAMESPACE

class QPulseAudioDeviceInfo : public QAudioDevicePrivate
{
public:
    QPulseAudioDeviceInfo(const char *device, const char *description, bool isDefault, QAudioDevice::Mode mode);
    ~QPulseAudioDeviceInfo() {}
};

QT_END_NAMESPACE

#endif

