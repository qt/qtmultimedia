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

#ifndef QABSTRACTMEDIAOBJECT_P_H
#define QABSTRACTMEDIAOBJECT_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qbytearray.h>
#include <QtCore/qset.h>
#include <QtCore/qtimer.h>

#include "qmediaobject.h"
#include "private/qobject_p.h"

QT_BEGIN_NAMESPACE


class QMetaDataReaderControl;
class QMediaAvailabilityControl;

#define Q_DECLARE_NON_CONST_PUBLIC(Class) \
    inline Class* q_func() { return static_cast<Class *>(q_ptr); } \
    friend class Class;


class QMediaObjectPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QMediaObject)

public:
    QMediaObjectPrivate() : service(nullptr), metaDataControl(nullptr), availabilityControl(nullptr), notifyTimer(nullptr) {}
    virtual ~QMediaObjectPrivate() {}

    void _q_notify();
    void _q_availabilityChanged();

    QMediaService *service;
    QMetaDataReaderControl *metaDataControl;
    QMediaAvailabilityControl *availabilityControl;

    QTimer* notifyTimer;
    QSet<int> notifyProperties;
};

QT_END_NAMESPACE


#endif
