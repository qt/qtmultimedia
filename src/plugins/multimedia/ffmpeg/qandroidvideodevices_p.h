// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QFANDROIDVIDEODEVICES_H
#define QFANDROIDVIDEODEVICES_H

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

#include <QObject>
#include <QtMultimedia/private/qplatformvideodevices_p.h>

class QAndroidVideoDevices : public QPlatformVideoDevices
{
    Q_OBJECT
public:
    using QPlatformVideoDevices::QPlatformVideoDevices;

protected:
    QList<QCameraDevice> findVideoInputs() const override;
};

#endif // QFANDROIDVIDEODEVICES_H
