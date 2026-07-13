// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QTMULTIMEDIAPRIVATEQMLHELPER_H
#define QTMULTIMEDIAPRIVATEQMLHELPER_H

#include <QtCore/qobject.h>
#include <QtCore/qstring.h>

#include <QtMultimedia/private/qplatformmediaintegration_p.h>

#include <QtQml/qqmlregistration.h>

class QtMultimediaPrivateQmlHelper : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(QtMultimediaPrivate)
    QML_SINGLETON

    // The name of the active media backend, e.g. "ffmpeg", "darwin", "gstreamer", "android".
    Q_PROPERTY(QString mediaBackendName READ mediaBackendName CONSTANT)

public:
    [[nodiscard]] QString mediaBackendName() const
    {
        return QString(QPlatformMediaIntegration::instance()->name());
    }
};

#endif // QTMULTIMEDIAPRIVATEQMLHELPER_H
