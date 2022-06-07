// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
