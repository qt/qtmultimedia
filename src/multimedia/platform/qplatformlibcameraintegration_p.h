// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QPLATFORMLIBCAMERAINTEGRATION_P_H
#define QPLATFORMLIBCAMERAINTEGRATION_P_H

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

#include <QtMultimedia/qtmultimediaglobal.h>
#include <QtCore/private/qexpected_p.h>
#include <QtCore/qobject.h>
#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE

class QCamera;
class QPlatformCamera;
class QPlatformMediaIntegration;
class QPlatformVideoDevices;

namespace QLibCamera {

class Q_MULTIMEDIA_EXPORT QPlatformLibCameraIntegration : public QObject
{
    Q_OBJECT
public:
    explicit QPlatformLibCameraIntegration(QObject *parent = nullptr);
    ~QPlatformLibCameraIntegration() override;

    virtual QPlatformVideoDevices *createVideoDevices(QPlatformMediaIntegration *parent) = 0;
    virtual q23::expected<QPlatformCamera *, QString> createCamera(QCamera *) = 0;
};

Q_MULTIMEDIA_EXPORT QPlatformLibCameraIntegration *libCameraIntegration();

} // namespace QLibCamera

QT_END_NAMESPACE

#endif // QPLATFORMLIBCAMERAINTEGRATION_P_H
