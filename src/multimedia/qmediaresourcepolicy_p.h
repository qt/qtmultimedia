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

#ifndef QMEDIARESOURCEPOLICY_H
#define QMEDIARESOURCEPOLICY_H

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
#include "qtmultimediaglobal.h"

QT_BEGIN_NAMESPACE

class Q_MULTIMEDIA_EXPORT QMediaResourcePolicy
{
public:
    //a dummy object will always be provided if the interfaceId is not supported
    template<typename T>
    static T* createResourceSet();
    static void destroyResourceSet(QObject* resourceSet);
private:
    static QObject* createResourceSet(const QString& interfaceId);
};

template<typename T>
T* QMediaResourcePolicy::createResourceSet()
{
    return qobject_cast<T*>(QMediaResourcePolicy::createResourceSet(T::iid()));
}

QT_END_NAMESPACE

#endif // QMEDIARESOURCEPOLICY_H
