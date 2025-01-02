// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QMEDIAENUMDEBUG_H
#define QMEDIAENUMDEBUG_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#include <QtCore/qmetaobject.h>
#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

#ifndef QT_NO_DEBUG_STREAM

#define Q_MEDIA_ENUM_DEBUG(Class,Enum) \
inline QDebug operator<<(QDebug dbg, Class::Enum value) \
{ \
    int index = Class::staticMetaObject.indexOfEnumerator(#Enum); \
    dbg.nospace() << #Class << "::" << Class::staticMetaObject.enumerator(index).valueToKey(value); \
    return dbg.space(); \
}

#else

#define Q_MEDIA_ENUM_DEBUG(Class,Enum)

#endif //QT_NO_DEBUG_STREAM

QT_END_NAMESPACE

#endif

