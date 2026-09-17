// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qfileutil_p.h"
#include <QtCore/qfile.h>
#include <QtCore/qdiriterator.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE

using namespace Qt::Literals;

bool copyAllFiles(const QDir &source, const QDir &dest)
{
    if (!source.exists() || !dest.exists())
        return false;

    QDirIterator it(source, QDirIterator::Subdirectories);
    bool success = true;
    while (it.hasNext()) {
        QFileInfo file{ it.next() };
        const QString relativePath = source.relativeFilePath(file.absoluteFilePath());
        const QString destination = dest.absolutePath() + u"/"_s + relativePath;

        if (file.isFile()) {
            if (QFile::exists(destination))
                if (!QFile::remove(destination))
                    success = false;

            if (!QFile::copy(file.absoluteFilePath(), destination))
                success = false;

        } else if (file.isDir()) {
            if (!QDir(destination).exists())
                if (!dest.mkpath(relativePath))
                    success = false;
        }
    }

    return success;
}

std::unique_ptr<QTemporaryFile> copyResourceToTemporaryFile(const QUrl &resource,
                                                            const QString &fileTemplate)
{
    QString resourcePath;
    if (resource.scheme() == "qrc"_L1)
        resourcePath = u':' + resource.path();
    else if (resource.isLocalFile())
        resourcePath = resource.toLocalFile();
    else
        return nullptr;

    QFile resourceFile(resourcePath);
    if (!resourceFile.open(QIODeviceBase::ReadOnly))
        return nullptr;

    auto temporaryFile = std::make_unique<QTemporaryFile>(fileTemplate);
    if (!temporaryFile->open())
        return nullptr;

    if (temporaryFile->write(resourceFile.readAll()) < 0)
        return nullptr;

    temporaryFile->close();
    return temporaryFile;
}

QT_END_NAMESPACE
