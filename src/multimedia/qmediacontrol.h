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

#ifndef QABSTRACTMEDIACONTROL_H
#define QABSTRACTMEDIACONTROL_H

#include <QtMultimedia/qtmultimediaglobal.h>

#include <QtCore/qobject.h>
#include <QtCore/qstring.h>
#include <QtCore/qvariant.h>


QT_BEGIN_NAMESPACE


class QMediaControlPrivate;
class Q_MULTIMEDIA_EXPORT QMediaControl : public QObject
{
    Q_OBJECT

public:
    ~QMediaControl();

protected:
    explicit QMediaControl(QObject *parent = nullptr);
    explicit QMediaControl(QMediaControlPrivate &dd, QObject *parent = nullptr);

    QMediaControlPrivate *d_ptr;

private:
    Q_DECLARE_PRIVATE(QMediaControl)
};

template <typename T> const char *qmediacontrol_iid() { return nullptr; }

#define Q_MEDIA_DECLARE_CONTROL(Class, IId) \
    template <> inline const char *qmediacontrol_iid<Class *>() { return IId; }

QT_END_NAMESPACE


#endif  // QABSTRACTMEDIACONTROL_H
