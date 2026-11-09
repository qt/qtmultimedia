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

enum class DmaBufStrategy : uint8_t {
    // The node takes server-allocated buffers, which are mapped and copied.
    None,
    // The stream allocates the DMA-BUFs and the node imports them (v4l2). A libcamera node accepts
    // imported buffers but never fills them, so it cannot use this.
    ClientAllocated,
    // The node allocates plain V4L2 or dma-heap buffers and advertises no modifier, so they are
    // linear (libcamera).
    NodeAllocated,
};

Q_MULTIMEDIA_EXPORT DmaBufStrategy dmaBufStrategy(const PwPropertyDict &deviceProperties);

// Where the camera faces, for the device.api values that report it.
Q_MULTIMEDIA_EXPORT QCameraDevice::Position cameraPosition(const PwPropertyDict &nodeProperties);

} // namespace QtPipeWire

QT_END_NAMESPACE

#endif // QPIPEWIRE_CAMERANODE_P_H
