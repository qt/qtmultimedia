// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qmediastoragelocation_p.h"

#include <QStandardPaths>
#include <QUrl>

QT_BEGIN_NAMESPACE

QDir QMediaStorageLocation::defaultDirectory(QStandardPaths::StandardLocation type)
{
    QStringList dirCandidates;

#if QT_CONFIG(mmrenderer)
    dirCandidates << QLatin1String("shared/camera");
#endif

    dirCandidates << QStandardPaths::writableLocation(type);
    dirCandidates << QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    dirCandidates << QDir::homePath();
    dirCandidates << QDir::currentPath();
    dirCandidates << QDir::tempPath();

    for (const QString &path : std::as_const(dirCandidates)) {
        QDir dir(path);
        if (dir.exists() && QFileInfo(path).isWritable())
            return dir;
    }

    return QDir();
}

static QString generateFileName(const QDir &dir, const QString &prefix, const QString &extension)
{
    // The extension may be empty if Qt is built without the MIME type feature.
    int lastMediaIndex = 0;
    const QStringView maybeDot = !extension.isEmpty() && !extension.startsWith(u'.') ? u"." : u"";
    const auto filesList =
            dir.entryList({ QStringView(u"%1*%2%3").arg(prefix, maybeDot, extension) });
    for (const QString &fileName : filesList) {
        const qsizetype mediaIndexSize =
                fileName.size() - prefix.size() - extension.size() - maybeDot.size();
        const int mediaIndex = QStringView{ fileName }.mid(prefix.size(), mediaIndexSize).toInt();
        lastMediaIndex = qMax(lastMediaIndex, mediaIndex);
    }

    const QString newMediaIndexStr = QStringLiteral("%1").arg(lastMediaIndex + 1, 4, 10, QLatin1Char(u'0'));
    const QString name = prefix + newMediaIndexStr + maybeDot + extension;

    return dir.absoluteFilePath(name);
}


QString QMediaStorageLocation::generateFileName(const QString &requestedName,
                                                QStandardPaths::StandardLocation type,
                                                const QString &extension)
{
    using namespace Qt::StringLiterals;

    if (QUrl(requestedName).scheme() == "content"_L1)
        return requestedName;

    auto prefix = "clip_"_L1;
    switch (type) {
        case QStandardPaths::PicturesLocation: prefix = "image_"_L1; break;
        case QStandardPaths::MoviesLocation: prefix = "video_"_L1; break;
        case QStandardPaths::MusicLocation: prefix = "record_"_L1; break;
        default: break;
    }

    if (requestedName.isEmpty())
        return generateFileName(defaultDirectory(type), prefix, extension);

    QString path = requestedName;

    const QFileInfo fileInfo{ path };

    if (fileInfo.isRelative() && QUrl(path).isRelative())
        path = defaultDirectory(type).absoluteFilePath(path);

    if (fileInfo.isDir())
        return generateFileName(QDir(path), prefix, extension);

    if (fileInfo.suffix().isEmpty() && !extension.isEmpty()) {
        // File does not have an extension, so add the suggested one
        if (!path.endsWith(u'.'))
            path.append(u'.');
        path.append(extension);
    }
    return path;
}

QT_END_NAMESPACE
