// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QGSTREAMER_QRC_HANDLER_H
#define QGSTREAMER_QRC_HANDLER_H

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

void qGstRegisterQRCHandler(GstPlugin *plugin);

QT_END_NAMESPACE

#endif
