/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qandroidintegration_p.h"
#include "qandroiddevicemanager_p.h"
#include "private/qandroidmediaservice_p.h"
#include "private/qandroidglobal_p.h"
#include "private/qandroidcaptureservice_p.h"
#include "private/androidmediaplayer_p.h"
#include "private/qandroidcamerasession_p.h"
#include "private/androidsurfacetexture_p.h"
#include "private/androidsurfaceview_p.h"
#include "private/androidcamera_p.h"
#include "private/androidmediarecorder_p.h"

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(qtAndroidMediaPlugin, "qt.multimedia.android")

QAndroidIntegration::QAndroidIntegration()
{

}

QAndroidIntegration::~QAndroidIntegration()
{
    delete m_manager;
}

QMediaPlatformDeviceManager *QAndroidIntegration::deviceManager()
{
    if (!m_manager)
        m_manager = new QAndroidDeviceManager();
    return m_manager;
}

QMediaPlatformCaptureInterface *QAndroidIntegration::createCaptureInterface(QMediaRecorder::CaptureMode mode)
{
    return new QAndroidCaptureService(mode);
}

QMediaPlatformPlayerInterface *QAndroidIntegration::createPlayerInterface()
{
    return new QAndroidMediaService();
}

Q_DECL_EXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void * /*reserved*/)
{
    static bool initialized = false;
    if (initialized)
        return JNI_VERSION_1_6;
    initialized = true;

    QT_USE_NAMESPACE
    typedef union {
        JNIEnv *nativeEnvironment;
        void *venv;
    } UnionJNIEnvToVoid;

    UnionJNIEnvToVoid uenv;
    uenv.venv = NULL;

    if (vm->GetEnv(&uenv.venv, JNI_VERSION_1_6) != JNI_OK)
        return JNI_ERR;

    JNIEnv *jniEnv = uenv.nativeEnvironment;

    if (!AndroidMediaPlayer::initJNI(jniEnv) ||
        !AndroidCamera::initJNI(jniEnv) ||
        !AndroidMediaRecorder::initJNI(jniEnv) ||
        !AndroidSurfaceHolder::initJNI(jniEnv)) {
        return JNI_ERR;
    }

    AndroidSurfaceTexture::initJNI(jniEnv);

    return JNI_VERSION_1_6;
}

QT_END_NAMESPACE
