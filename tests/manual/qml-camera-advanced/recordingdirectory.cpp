// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "recordingdirectory.h"

#include <QtCore/qdir.h>
#include <QtCore/qdebug.h>
#include <QtCore/qstandardpaths.h>

QString RecordingDirectory::ensureMoviesLocation() const
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
    if (path.isEmpty() || !QDir().mkpath(path)) {
        qWarning() << "Could not create the movies directory" << path;
        return {};
    }
    return path;
}
