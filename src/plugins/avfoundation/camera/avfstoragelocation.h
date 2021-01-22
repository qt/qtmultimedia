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

#ifndef AVFSTORAGE_H
#define AVFSTORAGE_H

#include "qtmultimediaglobal.h"

#include <QtCore/qdir.h>
#include <QtMultimedia/qcamera.h>

QT_BEGIN_NAMESPACE

class AVFStorageLocation
{
public:
    AVFStorageLocation();
    ~AVFStorageLocation();

    QString generateFileName(const QString &requestedName,
                             QCamera::CaptureMode mode,
                             const QString &prefix,
                             const QString &ext) const;


    QDir defaultDir(QCamera::CaptureMode mode) const;
    QString generateFileName(const QString &prefix, const QDir &dir, const QString &ext) const;

private:
    mutable QMap<QString, int> m_lastUsedIndex;
};

QT_END_NAMESPACE

#endif
