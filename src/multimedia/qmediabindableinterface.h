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

#ifndef QMEDIABINDABLEINTERFACE_H
#define QMEDIABINDABLEINTERFACE_H

#include <QtMultimedia/qmediaobject.h>

QT_BEGIN_NAMESPACE


class QMediaObject;

class Q_MULTIMEDIA_EXPORT QMediaBindableInterface
{
public:
    virtual ~QMediaBindableInterface();

    virtual QMediaObject *mediaObject() const = 0;

protected:
    friend class QMediaObject;
    virtual bool setMediaObject(QMediaObject *object) = 0;
};

#define QMediaBindableInterface_iid \
    "org.qt-project.qt.mediabindable/5.0"
Q_DECLARE_INTERFACE(QMediaBindableInterface, QMediaBindableInterface_iid)

QT_END_NAMESPACE


#endif  // QMEDIABINDABLEINTERFACE_H
