// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPIPEWIRE_CAMERANODE_P_H
#define QPIPEWIRE_CAMERANODE_P_H

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

#include <QtMultimedia/private/qpipewire_propertydict_p.h>
#include <QtMultimedia/private/qtmultimediaglobal_p.h>
#include <QtMultimedia/qcameradevice.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE

namespace QtPipeWire {

Q_MULTIMEDIA_EXPORT bool isCameraNode(const PwPropertyDict &nodeProperties);

Q_MULTIMEDIA_EXPORT QByteArray cameraDeviceId(const PwPropertyDict &nodeProperties);
Q_MULTIMEDIA_EXPORT QString cameraDescription(const PwPropertyDict &nodeProperties,
                                              const PwPropertyDict &deviceProperties);

// Where the camera faces, for the device.api values that report it.
Q_MULTIMEDIA_EXPORT QCameraDevice::Position cameraPosition(const PwPropertyDict &nodeProperties);

} // namespace QtPipeWire

QT_END_NAMESPACE

#endif // QPIPEWIRE_CAMERANODE_P_H
