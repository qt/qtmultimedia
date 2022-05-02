/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
******************************************************************************/

#include "androidmultimediautils_p.h"

#include <QtCore/qjniobject.h>

QT_BEGIN_NAMESPACE


void AndroidMultimediaUtils::enableOrientationListener(bool enable)
{
    QJniObject::callStaticMethod<void>("org/qtproject/qt/android/multimedia/QtMultimediaUtils",
                                       "enableOrientationListener",
                                       "(Z)V",
                                       enable);
}

int AndroidMultimediaUtils::getDeviceOrientation()
{
    return QJniObject::callStaticMethod<jint>("org/qtproject/qt/android/multimedia/QtMultimediaUtils",
                                              "getDeviceOrientation");
}

QString AndroidMultimediaUtils::getDefaultMediaDirectory(MediaType type)
{
    QJniObject path = QJniObject::callStaticObjectMethod(
                                   "org/qtproject/qt/android/multimedia/QtMultimediaUtils",
                                   "getDefaultMediaDirectory",
                                   "(I)Ljava/lang/String;",
                                   jint(type));
    return path.toString();
}

void AndroidMultimediaUtils::registerMediaFile(const QString &file)
{
    QJniObject::callStaticMethod<void>("org/qtproject/qt/android/multimedia/QtMultimediaUtils",
                                       "registerMediaFile",
                                       "(Ljava/lang/String;)V",
                                       QJniObject::fromString(file).object());
}

QT_END_NAMESPACE
