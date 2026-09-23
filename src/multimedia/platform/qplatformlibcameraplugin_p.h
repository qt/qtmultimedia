// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QPLATFORMLIBCAMERAPLUGIN_P_H
#define QPLATFORMLIBCAMERAPLUGIN_P_H

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
#include <QtCore/private/qglobal_p.h>
#include <QtCore/qfactoryinterface.h>
#include <QtCore/qplugin.h>

QT_BEGIN_NAMESPACE

namespace QLibCamera {

class QPlatformLibCameraIntegration;

#define QPlatformLibCameraPlugin_iid "org.qt-project.Qt.QPlatformLibCameraPlugin"

class Q_MULTIMEDIA_EXPORT QPlatformLibCameraPlugin : public QObject
{
    Q_OBJECT
public:
    explicit QPlatformLibCameraPlugin(QObject *parent = nullptr);
    ~QPlatformLibCameraPlugin() override;

    virtual std::unique_ptr<QPlatformLibCameraIntegration> create() = 0;
};

} // namespace QLibCamera

QT_END_NAMESPACE

#endif // QPLATFORMLIBCAMERAPLUGIN_P_H
