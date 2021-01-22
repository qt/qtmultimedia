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

#ifndef QABSTRACTMEDIASERVICE_H
#define QABSTRACTMEDIASERVICE_H

#include <QtMultimedia/qtmultimediaglobal.h>
#include <QtCore/qobject.h>
#include <QtCore/qstringlist.h>

#include <QtMultimedia/qmediacontrol.h>

QT_BEGIN_NAMESPACE


class QMediaServicePrivate;
class Q_MULTIMEDIA_EXPORT QMediaService : public QObject
{
    Q_OBJECT

public:
    ~QMediaService();

    virtual QMediaControl* requestControl(const char *name) = 0;

    template <typename T> inline T requestControl() {
        if (QMediaControl *control = requestControl(qmediacontrol_iid<T>())) {
            if (T typedControl = qobject_cast<T>(control))
                return typedControl;
            releaseControl(control);
        }
        return 0;
    }

    virtual void releaseControl(QMediaControl *control) = 0;

protected:
    QMediaService(QObject* parent);
    QMediaService(QMediaServicePrivate &dd, QObject *parent);

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QMediaServicePrivate *d_ptr_deprecated;
#endif

private:
    Q_DECLARE_PRIVATE(QMediaService)
};

QT_END_NAMESPACE


#endif  // QABSTRACTMEDIASERVICE_H

