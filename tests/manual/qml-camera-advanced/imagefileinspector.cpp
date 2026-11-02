// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "imagefileinspector.h"

#include <QtCore/qfileinfo.h>
#include <QtGui/qimagereader.h>

using namespace Qt::StringLiterals;

QVariantMap ImageFileInspector::inspect(const QString &path) const
{
    QVariantMap info;
    info[u"valid"_s] = false;

    const QFileInfo fileInfo(path);
    if (!fileInfo.exists())
        return info;

    QImageReader reader(path);
    const QSize size = reader.size();
    info[u"valid"_s] = size.isValid();
    info[u"resolution"_s] = size;
    info[u"format"_s] = QString::fromLatin1(reader.format());
    info[u"fileSize"_s] = fileInfo.size();
    return info;
}
