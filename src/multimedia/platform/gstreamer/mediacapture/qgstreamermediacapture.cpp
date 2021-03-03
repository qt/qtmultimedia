/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "qgstreamermediacapture_p.h"
#include "qgstreamercapturesession_p.h"
#include "qgstreamermediarecorder_p.h"
#include "qgstreamercamera_p.h"
#include <private/qgstreamerbushelper_p.h>

#include "qgstreamercameraimagecapture_p.h"

#include <private/qgstreamervideorenderer_p.h>
#include <private/qgstreamervideowindow_p.h>

QT_BEGIN_NAMESPACE

QGstreamerMediaCapture::QGstreamerMediaCapture(QMediaRecorder::CaptureMode mode)
{
    if (mode == QMediaRecorder::AudioOnly) {
        m_captureSession = new QGstreamerCaptureSession(QGstreamerCaptureSession::Audio, this);
    } else {
        m_captureSession = new QGstreamerCaptureSession(QGstreamerCaptureSession::AudioAndVideo, this);
        m_cameraControl = new QGstreamerCamera(m_captureSession);
    }
}

QGstreamerMediaCapture::~QGstreamerMediaCapture() = default;

QPlatformCamera *QGstreamerMediaCapture::cameraControl()
{
    return m_cameraControl;
}

QPlatformCameraImageCapture *QGstreamerMediaCapture::imageCaptureControl()
{
    return m_captureSession->imageCaptureControl();
}

QPlatformMediaRecorder *QGstreamerMediaCapture::mediaRecorderControl()
{
    return m_captureSession->recorderControl();
}

QT_END_NAMESPACE
