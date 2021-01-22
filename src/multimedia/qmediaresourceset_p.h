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

#ifndef QMEDIARESOURCESET_P_H
#define QMEDIARESOURCESET_P_H

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
#include <QObject>
#include <qtmultimediaglobal.h>

QT_BEGIN_NAMESPACE

#define QMediaPlayerResourceSetInterface_iid \
    "org.qt-project.qt.mediaplayerresourceset/5.0"

class Q_MULTIMEDIA_EXPORT QMediaPlayerResourceSetInterface : public QObject
{
    Q_OBJECT
public:
    virtual bool isVideoEnabled() const = 0;
    virtual bool isGranted() const = 0;
    virtual bool isAvailable() const = 0;

    virtual void acquire() = 0;
    virtual void release() = 0;
    virtual void setVideoEnabled(bool enabled) = 0;

    static QString iid();

Q_SIGNALS:
    void resourcesGranted();
    void resourcesLost();
    void resourcesDenied();
    void resourcesReleased();
    void availabilityChanged(bool available);

protected:
    QMediaPlayerResourceSetInterface(QObject *parent = nullptr);
};

QT_END_NAMESPACE

#endif // QMEDIARESOURCESET_P_H
