// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QGSTREAMER_QIODEVICE_HANDLER_H
#define QGSTREAMER_QIODEVICE_HANDLER_H

#include <QtCore/qglobal.h>
#include <gst/gstplugin.h>

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

QT_BEGIN_NAMESPACE

class QIODevice;
class QUrl;

void qGstRegisterQIODeviceHandler(GstPlugin *plugin);

// Note: if the QIODevice is sequential, the QUrl is only allowed to be used in one location.
// if the QUrl is not sequential, it can be passed to multiple destinations
QUrl qGstRegisterQIODevice(QIODevice *);

QT_END_NAMESPACE

#endif
