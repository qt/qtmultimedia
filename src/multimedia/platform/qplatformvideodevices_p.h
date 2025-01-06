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
#include <qcameradevice.h>
#include <qobject.h>

#include <private/qcachedvalue_p.h>

QT_BEGIN_NAMESPACE

class QCameraDevice;
class QPlatformMediaIntegration;

class Q_MULTIMEDIA_EXPORT QPlatformVideoDevices : public QObject
{
    Q_OBJECT

    QT_DEFINE_TAG_STRUCT(PrivateTag);
public:
    QPlatformVideoDevices(QPlatformMediaIntegration *integration);

    ~QPlatformVideoDevices() override;

    QList<QCameraDevice> videoInputs() const;

protected:
    virtual QList<QCameraDevice> findVideoInputs() const = 0;

    void onVideoInputsChanged();

Q_SIGNALS:
    void videoInputsChanged(PrivateTag);

protected:
    QPlatformMediaIntegration *m_integration = nullptr;
    mutable QCachedValue<QList<QCameraDevice>> m_videoInputs;
};

QT_END_NAMESPACE

#endif
