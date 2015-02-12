/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd and/or its subsidiary(-ies).
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qwinrtcameraservice.h"
#include "qwinrtcameracontrol.h"
#include "qwinrtcamerainfocontrol.h"

#include <QtCore/QCoreApplication>
#include <QtCore/qfunctions_winrt.h>
#include <QtCore/QPointer>
#include <QtMultimedia/QCameraImageCaptureControl>
#include <QtMultimedia/QVideoRendererControl>
#include <QtMultimedia/QVideoDeviceSelectorControl>

QT_USE_NAMESPACE

class QWinRTCameraServicePrivate
{
public:
    QPointer<QWinRTCameraControl> cameraControl;
    QPointer<QWinRTCameraInfoControl> cameraInfoControl;
};

QWinRTCameraService::QWinRTCameraService(QObject *parent)
    : QMediaService(parent), d_ptr(new QWinRTCameraServicePrivate)
{
}

QMediaControl *QWinRTCameraService::requestControl(const char *name)
{
    Q_D(QWinRTCameraService);

    if (qstrcmp(name, QCameraControl_iid) == 0) {
        if (!d->cameraControl)
            d->cameraControl = new QWinRTCameraControl(this);
        return d->cameraControl;
    }

    if (qstrcmp(name, QVideoRendererControl_iid) == 0) {
        if (d->cameraControl)
            return d->cameraControl->videoRenderer();
    }

    if (qstrcmp(name, QVideoDeviceSelectorControl_iid) == 0) {
        if (d->cameraControl)
            return d->cameraControl->videoDeviceSelector();
    }

    if (qstrcmp(name, QCameraInfoControl_iid) == 0) {
        if (!d->cameraInfoControl)
            d->cameraInfoControl = new QWinRTCameraInfoControl(this);
        return d->cameraInfoControl;
    }

    if (qstrcmp(name, QCameraImageCaptureControl_iid) == 0) {
        if (d->cameraControl)
            return d->cameraControl->imageCaptureControl();
    }

    return Q_NULLPTR;
}

void QWinRTCameraService::releaseControl(QMediaControl *control)
{
    Q_UNUSED(control);
}
