// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef IMAGEFILEINSPECTOR_H
#define IMAGEFILEINSPECTOR_H

#include <QtCore/qobject.h>
#include <QtCore/qvariant.h>
#include <QtQml/qqmlregistration.h>

class ImageFileInspector : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    // Returns { valid, resolution, format, fileSize }. Only the header is read. fileSize is in
    // bytes.
    [[nodiscard]] Q_INVOKABLE QVariantMap inspect(const QString &path) const;
};

#endif // IMAGEFILEINSPECTOR_H
