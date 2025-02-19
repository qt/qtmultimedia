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

#include <QtMultimedia/qaudio.h>
#include <QtMultimedia/qaudiodevice.h>
#include <QtMultimedia/private/qaudiodevice_p.h>

#include <QtGstreamerMediaPluginImpl/private/qgst_handle_types_p.h>

#include <gst/gst.h>

QT_BEGIN_NAMESPACE

class QGStreamerCustomAudioDeviceInfo : public QAudioDevicePrivate
{
public:
    QGStreamerCustomAudioDeviceInfo(const QByteArray &gstreamerPipeline, QAudioDevice::Mode mode);
};

bool isCustomAudioDevice(const QAudioDevicePrivate *device);
bool isCustomAudioDevice(const QAudioDevice &device);

QAudioDevice qMakeCustomGStreamerAudioInput(const QByteArray &gstreamerPipeline);
QAudioDevice qMakeCustomGStreamerAudioOutput(const QByteArray &gstreamerPipeline);

QT_END_NAMESPACE

#endif

