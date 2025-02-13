// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QV4L2CAMERADEVICES_P_H
#define QV4L2CAMERADEVICES_P_H

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

#include <QtMultimedia/private/qplatformvideodevices_p.h>
#include <QtMultimedia/private/qplatformmediaintegration_p.h>

#include <qfilesystemwatcher.h>

QT_BEGIN_NAMESPACE

class QV4L2CameraDevices : public QPlatformVideoDevices
{
    Q_OBJECT
public:
    QV4L2CameraDevices(QPlatformMediaIntegration *integration);

public Q_SLOTS:
    void checkCameras();

protected:
    QList<QCameraDevice> findVideoInputs() const override;

private:
    bool doCheckCameras();

private:
    QList<QCameraDevice> m_cameras;
    QFileSystemWatcher m_deviceWatcher;
};

QT_END_NAMESPACE

#endif // QV4L2CAMERADEVICES_P_H
