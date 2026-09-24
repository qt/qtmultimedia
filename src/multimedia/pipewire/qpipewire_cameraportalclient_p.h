// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPIPEWIRE_CAMERAPORTALCLIENT_P_H
#define QPIPEWIRE_CAMERAPORTALCLIENT_P_H

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

#include <QtMultimedia/private/qtmultimediaglobal_p.h>
#include <QtCore/private/qexpected_p.h>
#include <QtCore/qstring.h>

#include <chrono>

QT_BEGIN_NAMESPACE

class QDBusUnixFileDescriptor;

namespace QtPipeWire {

// Negotiates camera access through org.freedesktop.portal.Camera and returns
// the PipeWire remote file descriptor OpenPipeWireRemote() hands out, or an
// error if access was denied, the portal is not present, or 'timeout' elapsed
// first.
q23::expected<QDBusUnixFileDescriptor, QString>
requestCameraPortalFd(std::chrono::milliseconds timeout);

} // namespace QtPipeWire

QT_END_NAMESPACE

#endif // QPIPEWIRE_CAMERAPORTALCLIENT_P_H
