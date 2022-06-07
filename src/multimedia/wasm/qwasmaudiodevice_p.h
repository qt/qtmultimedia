// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWASMAUDIODEVICEINFO_H
#define QWASMAUDIODEVICEINFO_H

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

#include "private/qaudiodevice_p.h"

QT_BEGIN_NAMESPACE

class QWasmAudioDevice : public QAudioDevicePrivate
{
public:
    QWasmAudioDevice(const char *device, const char *description, bool isDefault, QAudioDevice::Mode mode);

};

QT_END_NAMESPACE

#endif // QWASMAUDIODEVICEINFO_H
