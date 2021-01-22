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

#include "androidmultimediautils.h"

#include <QtCore/private/qjni_p.h>

QT_BEGIN_NAMESPACE


void AndroidMultimediaUtils::enableOrientationListener(bool enable)
{
    QJNIObjectPrivate::callStaticMethod<void>("org/qtproject/qt5/android/multimedia/QtMultimediaUtils",
                                              "enableOrientationListener",
                                              "(Z)V",
                                              enable);
}

int AndroidMultimediaUtils::getDeviceOrientation()
{
    return QJNIObjectPrivate::callStaticMethod<jint>("org/qtproject/qt5/android/multimedia/QtMultimediaUtils",
                                                     "getDeviceOrientation");
}

QString AndroidMultimediaUtils::getDefaultMediaDirectory(MediaType type)
{
    QJNIObjectPrivate path = QJNIObjectPrivate::callStaticObjectMethod("org/qtproject/qt5/android/multimedia/QtMultimediaUtils",
                                                                       "getDefaultMediaDirectory",
                                                                       "(I)Ljava/lang/String;",
                                                                       jint(type));
    return path.toString();
}

void AndroidMultimediaUtils::registerMediaFile(const QString &file)
{
    QJNIObjectPrivate::callStaticMethod<void>("org/qtproject/qt5/android/multimedia/QtMultimediaUtils",
                                              "registerMediaFile",
                                              "(Ljava/lang/String;)V",
                                              QJNIObjectPrivate::fromString(file).object());
}

QT_END_NAMESPACE
