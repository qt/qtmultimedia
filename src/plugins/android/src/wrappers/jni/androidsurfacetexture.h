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

#ifndef ANDROIDSURFACETEXTURE_H
#define ANDROIDSURFACETEXTURE_H

#include <qobject.h>
#include <QtCore/private/qjni_p.h>

#include <QMatrix4x4>

QT_BEGIN_NAMESPACE

class AndroidSurfaceTexture : public QObject
{
    Q_OBJECT
public:
    explicit AndroidSurfaceTexture(quint32 texName);
    ~AndroidSurfaceTexture();

    jobject surfaceTexture();
    jobject surface();
    jobject surfaceHolder();
    inline bool isValid() const { return m_surfaceTexture.isValid(); }

    QMatrix4x4 getTransformMatrix();
    void release(); // API level 14
    void updateTexImage();

    void attachToGLContext(quint32 texName); // API level 16
    void detachFromGLContext(); // API level 16

    static bool initJNI(JNIEnv *env);

Q_SIGNALS:
    void frameAvailable();

private:
    void setOnFrameAvailableListener(const QJNIObjectPrivate &listener);

    QJNIObjectPrivate m_surfaceTexture;
    QJNIObjectPrivate m_surface;
    QJNIObjectPrivate m_surfaceHolder;
};

QT_END_NAMESPACE

#endif // ANDROIDSURFACETEXTURE_H
