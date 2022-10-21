/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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

    for (const QString &path : qAsConst(dirCandidates)) {
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
        auto mediaIndex = QStringView{fileName}.mid(prefix.length(), fileName.size() - prefix.length() - extension.length() - 1).toInt();
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
