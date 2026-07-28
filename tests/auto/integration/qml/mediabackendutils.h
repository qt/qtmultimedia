// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef MEDIABACKENDUTILS_QML_H
#define MEDIABACKENDUTILS_QML_H

#include <QtCore/qobject.h>

#include <QtQml/qqmlregistration.h>

class MediaBackendUtils : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(bool isMediaBackendPluginLoaded READ isMediaBackendPluginLoaded CONSTANT)
public:
    bool isMediaBackendPluginLoaded() const;
};

#endif // MEDIABACKENDUTILS_QML_H
