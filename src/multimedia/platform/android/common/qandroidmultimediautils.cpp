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

#include "qandroidmultimediautils_p.h"
#include "qandroidglobal_p.h"

#include <qlist.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/private/qandroidextras_p.h>

QT_BEGIN_NAMESPACE

int qt_findClosestValue(const QList<int> &list, int value)
{
    if (list.size() < 2)
        return 0;

    int begin = 0;
    int end = list.size() - 1;
    int pivot = begin + (end - begin) / 2;
    int v = list.at(pivot);

    while (end - begin > 1) {
        if (value == v)
            return pivot;

        if (value > v)
            begin = pivot;
        else
            end = pivot;

        pivot = begin + (end - begin) / 2;
        v = list.at(pivot);
    }

    return value - v >= list.at(pivot + 1) - value ? pivot + 1 : pivot;
}

bool qt_sizeLessThan(const QSize &s1, const QSize &s2)
{
    return s1.width() * s1.height() < s2.width() * s2.height();
}

QVideoFrameFormat::PixelFormat qt_pixelFormatFromAndroidImageFormat(AndroidCamera::ImageFormat f)
{
    switch (f) {
    case AndroidCamera::NV21:
        return QVideoFrameFormat::Format_NV21;
    case AndroidCamera::YV12:
        return QVideoFrameFormat::Format_YV12;
    case AndroidCamera::YUY2:
        return QVideoFrameFormat::Format_YUYV;
    case AndroidCamera::JPEG:
        return QVideoFrameFormat::Format_Jpeg;
    default:
        return QVideoFrameFormat::Format_Invalid;
    }
}

AndroidCamera::ImageFormat qt_androidImageFormatFromPixelFormat(QVideoFrameFormat::PixelFormat f)
{
    switch (f) {
    case QVideoFrameFormat::Format_NV21:
        return AndroidCamera::NV21;
    case QVideoFrameFormat::Format_YV12:
        return AndroidCamera::YV12;
    case QVideoFrameFormat::Format_YUYV:
        return AndroidCamera::YUY2;
    case QVideoFrameFormat::Format_Jpeg:
        return AndroidCamera::JPEG;
    default:
        return AndroidCamera::UnknownImageFormat;
    }
}

static bool androidRequestPermission(QtAndroidPrivate::PermissionType key)
{
    if (QNativeInterface::QAndroidApplication::sdkVersion() < 23)
        return true;

    // Permission already granted?
    if (QtAndroidPrivate::checkPermission(key).result() == QtAndroidPrivate::Authorized)
        return true;

    if (QtAndroidPrivate::requestPermission(key).result() != QtAndroidPrivate::Authorized)
        return false;

    return true;
}

static bool androidCheckPermission(QtAndroidPrivate::PermissionType key)
{
    if (QNativeInterface::QAndroidApplication::sdkVersion() < 23)
        return true;

    // Permission already granted?
    return (QtAndroidPrivate::checkPermission(key).result() == QtAndroidPrivate::Authorized);
}

bool qt_androidCheckCameraPermission()
{
    return androidCheckPermission(QtAndroidPrivate::Camera);
}

bool qt_androidCheckMicrophonePermission()
{
    return androidCheckPermission(QtAndroidPrivate::Microphone);
}

bool qt_androidRequestCameraPermission()
{
    if (!androidRequestPermission(QtAndroidPrivate::Camera)) {
        qCDebug(qtAndroidMediaPlugin, "Camera permission denied by user!");
        return false;
    }

    return true;
}

bool qt_androidRequestRecordingPermission()
{
    if (!androidRequestPermission(QtAndroidPrivate::Microphone)) {
        qCDebug(qtAndroidMediaPlugin, "Microphone permission denied by user!");
        return false;
    }

    return true;
}

bool qt_androidRequestWriteStoragePermission()
{
    if (!androidRequestPermission(QtAndroidPrivate::Storage)) {
        qCDebug(qtAndroidMediaPlugin, "Storage permission denied by user!");
        return false;
    }

    return true;
}

QT_END_NAMESPACE
