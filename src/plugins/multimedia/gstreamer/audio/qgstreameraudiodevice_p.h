// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QGSTREAMERAUDIODEVICEINFO_H
#define QGSTREAMERAUDIODEVICEINFO_H

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
#include <private/qaudiodevice_p.h>

#include <gst/gst.h>

QT_BEGIN_NAMESPACE

class QGStreamerAudioDeviceInfo : public QAudioDevicePrivate
{
public:
    QGStreamerAudioDeviceInfo(GstDevice *gstDevice, const QByteArray &device, QAudioDevice::Mode mode);
    ~QGStreamerAudioDeviceInfo();

    GstDevice *gstDevice = nullptr;
};

QT_END_NAMESPACE

#endif

