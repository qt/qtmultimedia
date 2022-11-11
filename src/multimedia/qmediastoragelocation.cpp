// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qmediastoragelocation_p.h"

#include <QStandardPaths>

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
    auto lastMediaIndex = 0;
    const auto list = dir.entryList({ QString::fromLatin1("%1*.%2").arg(prefix, extension) });
    for (const QString &fileName : list) {
        auto mediaIndex = QStringView{fileName}.mid(prefix.size(), fileName.size() - prefix.size() - extension.size() - 1).toInt();
        lastMediaIndex = qMax(lastMediaIndex, mediaIndex);
    }

    const QString name = QString::fromLatin1("%1%2.%3")
            .arg(prefix)
            .arg(lastMediaIndex + 1, 4, 10, QLatin1Char('0'))
            .arg(extension);

    return dir.absoluteFilePath(name);
}


QString QMediaStorageLocation::generateFileName(const QString &requestedName,
                                                QStandardPaths::StandardLocation type,
                                                const QString &extension)
{
    auto prefix = QLatin1String("clip_");
    switch (type) {
        case QStandardPaths::PicturesLocation: prefix = QLatin1String("image_"); break;
        case QStandardPaths::MoviesLocation: prefix = QLatin1String("video_"); break;
        case QStandardPaths::MusicLocation: prefix = QLatin1String("record_"); break;
        default: break;
    }

    if (requestedName.isEmpty())
        return generateFileName(defaultDirectory(type), prefix, extension);

    QString path = requestedName;

    if (QFileInfo(path).isRelative())
        path = defaultDirectory(type).absoluteFilePath(path);

    if (QFileInfo(path).isDir())
        return generateFileName(QDir(path), prefix, extension);

    if (!path.endsWith(extension))
        path.append(QString(QLatin1String(".%1")).arg(extension));

    return path;
}

QT_END_NAMESPACE
