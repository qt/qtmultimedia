// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QPLATFORMVIDEODEVICES_H
#define QPLATFORMVIDEODEVICES_H

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

#include <private/qtmultimediaglobal_p.h>
#include <qmediarecorder.h>
#include <qobject.h>

QT_BEGIN_NAMESPACE

class QPlatformMediaIntegration;

class Q_MULTIMEDIA_EXPORT QPlatformVideoDevices : public QObject
{
    Q_OBJECT
public:
    QPlatformVideoDevices(QPlatformMediaIntegration *integration)
        : m_integration(integration)
    {}

    ~QPlatformVideoDevices() override;

    virtual QList<QCameraDevice> videoDevices() const = 0;

Q_SIGNALS:
    void videoInputsChanged();

protected:
    QPlatformMediaIntegration *m_integration = nullptr;
};

QT_END_NAMESPACE

#endif
