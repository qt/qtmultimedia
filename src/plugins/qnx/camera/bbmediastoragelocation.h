/****************************************************************************
**
** Copyright (C) 2016 Research In Motion
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
#ifndef BBMEDIASTORAGELOCATION_H
#define BBMEDIASTORAGELOCATION_H

#include <QCamera>
#include <QDir>
#include <QHash>

QT_BEGIN_NAMESPACE

class BbMediaStorageLocation
{
public:
    BbMediaStorageLocation();

    QDir defaultDir(QCamera::CaptureMode mode) const;

    QString generateFileName(const QString &requestedName, QCamera::CaptureMode mode, const QString &prefix, const QString &extension) const;
    QString generateFileName(const QString &prefix, const QDir &dir, const QString &extension) const;

private:
    mutable QHash<QString, qint64> m_lastUsedIndex;
};

QT_END_NAMESPACE

#endif
