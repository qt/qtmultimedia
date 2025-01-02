// Copyright (C) 2016 Research In Motion
// Copyright (C) 2021 The Qt Company
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QNXAUDIODEVICE_P_H
#define QNXAUDIODEVICE_P_H

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

#include "private/qaudiosystem_p.h"
#include <private/qaudiodevice_p.h>

QT_BEGIN_NAMESPACE

class QnxAudioDeviceInfo : public QAudioDevicePrivate
{
public:
    QnxAudioDeviceInfo(const QByteArray &deviceName, QAudioDevice::Mode mode);
    ~QnxAudioDeviceInfo();

    bool isFormatSupported(const QAudioFormat &format) const;
};

QT_END_NAMESPACE

#endif
