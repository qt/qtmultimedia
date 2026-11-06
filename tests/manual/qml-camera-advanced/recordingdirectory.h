// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef RECORDINGDIRECTORY_H
#define RECORDINGDIRECTORY_H

#include <QtCore/qobject.h>
#include <QtCore/qstring.h>
#include <QtQml/qqmlregistration.h>

// QStandardPaths::writableLocation() may return a directory that doesn't exist, for example
// Documents/Movies on iOS. QML can't create directories itself.
class RecordingDirectory : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    // Creates the writable MoviesLocation if needed and returns its path.
    // Returns an empty string if the directory could not be created.
    [[nodiscard]] Q_INVOKABLE QString ensureMoviesLocation() const;
};

#endif // RECORDINGDIRECTORY_H
