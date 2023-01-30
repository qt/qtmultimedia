// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qandroidmultimediautils_p.h"
#include "qandroidglobal_p.h"

#include <qlist.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qpermissions.h>
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

static bool androidRequestPermission(const QString &permission)
{
    if (QNativeInterface::QAndroidApplication::sdkVersion() < 23)
        return true;

    // Permission already granted?
    if (QtAndroidPrivate::checkPermission(permission).result() == QtAndroidPrivate::Authorized)
        return true;

    if (QtAndroidPrivate::requestPermission(permission).result() != QtAndroidPrivate::Authorized)
        return false;

    return true;
}

static bool androidCheckPermission(const QPermission &permission)
{
    return qApp->checkPermission(permission) == Qt::PermissionStatus::Granted;
}

bool qt_androidCheckCameraPermission()
{
    const QCameraPermission permission;
    const auto granted = androidCheckPermission(permission);
    if (!granted)
        qCDebug(qtAndroidMediaPlugin, "Camera permission not granted!");
    return granted;
}

bool qt_androidCheckMicrophonePermission()
{
    const QMicrophonePermission permission;
    const auto granted = androidCheckPermission(permission);
    if (!granted)
        qCDebug(qtAndroidMediaPlugin, "Microphone permission not granted!");
    return granted;
}

bool qt_androidRequestWriteStoragePermission()
{
    if (!androidRequestPermission(QStringLiteral("android.permission.WRITE_EXTERNAL_STORAGE"))) {
        qCDebug(qtAndroidMediaPlugin, "Storage permission denied by user!");
        return false;
    }

    return true;
}

QT_END_NAMESPACE
