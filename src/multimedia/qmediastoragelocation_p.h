/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QMEDIASTORAGELOCATION_H
#define QMEDIASTORAGELOCATION_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <qtmultimediaglobal.h>
#include <QDir>
#include <QMap>
#include <QHash>
#include <QMutex>

QT_BEGIN_NAMESPACE

class Q_MULTIMEDIA_EXPORT QMediaStorageLocation
{
public:
    enum MediaType {
        Movies,
        Music,
        Pictures,
        Sounds
    };

    QMediaStorageLocation();

    void addStorageLocation(MediaType type, const QString &location);

    QDir defaultLocation(MediaType type) const;

    QString generateFileName(const QString &requestedName, MediaType type, const QString &prefix, const QString &extension) const;
    QString generateFileName(const QString &prefix, const QDir &dir, const QString &extension) const;

private:
    mutable QMutex m_mutex;
    mutable QHash<QString, qint64> m_lastUsedIndex;
    QMap<MediaType, QStringList> m_customLocations;
};

QT_END_NAMESPACE

#endif // QMEDIASTORAGELOCATION_H
