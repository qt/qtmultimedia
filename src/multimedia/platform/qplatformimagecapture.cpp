// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qplatformimagecapture_p.h"
#include <QtCore/qstringlist.h>

QT_BEGIN_NAMESPACE

QPlatformImageCapture::QPlatformImageCapture(QImageCapture *parent)
    : QObject(parent),
    m_imageCapture(parent)
{
}

QString QPlatformImageCapture::msgCameraNotReady()
{
    return QImageCapture::tr("Camera is not ready.");
}

QString QPlatformImageCapture::msgImageCaptureNotSet()
{
    return QImageCapture::tr("No instance of QImageCapture set on QMediaCaptureSession.");
}

QT_END_NAMESPACE

#include "moc_qplatformimagecapture_p.cpp"
