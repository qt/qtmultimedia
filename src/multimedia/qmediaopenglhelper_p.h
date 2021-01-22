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

#ifndef QMEDIAOPENGLHELPER_P_H
#define QMEDIAOPENGLHELPER_P_H

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

#include <QtGui/QOpenGLContext>

#if defined(Q_OS_WIN) && !defined(Q_OS_WINRT) && (defined(QT_OPENGL_ES_2) || defined(QT_OPENGL_DYNAMIC))
#include <EGL/egl.h>
#endif

QT_BEGIN_NAMESPACE

class QMediaOpenGLHelper
{
public:
    static bool isANGLE();
};

inline bool QMediaOpenGLHelper::isANGLE()
{
#ifdef Q_OS_WINRT
    return true;
#else
    bool isANGLE = false;

# if defined(Q_OS_WIN) && (defined(QT_OPENGL_ES_2) || defined(QT_OPENGL_DYNAMIC))
    if (QOpenGLContext::openGLModuleType() == QOpenGLContext::LibGLES) {
        // Although unlikely, technically LibGLES could mean a non-ANGLE EGL/GLES2 implementation too.
        // Verify that it is indeed ANGLE.
#  ifdef QT_OPENGL_ES_2_ANGLE_STATIC
        // ANGLE linked-in statically.
        isANGLE = true;
#  else
        // Qt configured with either -opengl es2 or -opengl desktop.
#   ifdef QT_DEBUG
        HMODULE eglHandle = LoadLibraryW(L"libEGLd.dll");
#   else
        HMODULE eglHandle = LoadLibraryW(L"libEGL.dll");
#   endif // QT_DEBUG
        if (eglHandle) {
            typedef EGLDisplay (EGLAPIENTRYP EglGetDisplay)(EGLNativeDisplayType display_id);
            typedef EGLBoolean (EGLAPIENTRYP EglInitialize)(EGLDisplay dpy, EGLint *major, EGLint *minor);
            typedef const char * (EGLAPIENTRYP EglQueryString)(EGLDisplay dpy, EGLint name);
            EglGetDisplay eglGetDisplay = (EglGetDisplay) GetProcAddress(eglHandle, "eglGetDisplay");
            EglInitialize eglInitialize = (EglInitialize) GetProcAddress(eglHandle, "eglInitialize");
            EglQueryString eglQueryString = (EglQueryString) GetProcAddress(eglHandle, "eglQueryString");
            if (eglGetDisplay && eglInitialize && eglQueryString) {
                // EGL may not be initialized at this stage.
                EGLDisplay dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
                eglInitialize(dpy, 0, 0);
                const char *vendorStr = eglQueryString(dpy, EGL_VERSION);
                isANGLE = vendorStr && strstr(vendorStr, "ANGLE");
            }
        }
#  endif // QT_OPENGL_ES_2_ANGLE_STATIC

    }
# endif // Q_OS_WIN && (QT_OPENGL_ES_2 || QT_OPENGL_DYNAMIC)

    return isANGLE;
#endif // Q_OS_WINRT
}

QT_END_NAMESPACE

#endif
