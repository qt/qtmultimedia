// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPIPEWIRE_CAMERADEVICES_P_H
#define QPIPEWIRE_CAMERADEVICES_P_H

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

#include <QtMultimedia/private/qplatformvideodevices_p.h>
#include <QtMultimedia/private/qtmultimediaglobal_p.h>
#include <QtMultimedia/qcameradevice.h>
#include <QtCore/qlist.h>
#include <QtCore/qchronotimer.h>

QT_BEGIN_NAMESPACE

namespace QtPipeWire {

class Q_MULTIMEDIA_EXPORT QCameraDevices : public QPlatformVideoDevices
{
public:
    explicit QCameraDevices(QPlatformMediaIntegration *integration);

    static bool isSupported();

    QList<QCameraDevice> findVideoInputs() const override;

private:
    QChronoTimer m_changeCompressionTimer{
        std::chrono::milliseconds(50),
    };
};

} // namespace QtPipeWire

QT_END_NAMESPACE

#endif // QPIPEWIRE_CAMERADEVICES_P_H
