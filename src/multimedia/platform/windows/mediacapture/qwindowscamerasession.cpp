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

#include "qwindowscamerasession_p.h"

#include "qwindowscamerareader_p.h"
#include "qwindowscamerafocus_p.h"
#include "qwindowscameraexposure_p.h"
#include "qwindowscameraimageprocessing_p.h"
#include <qvideosink.h>
#include <qdebug.h>

QT_BEGIN_NAMESPACE

QWindowsCameraSession::QWindowsCameraSession(QObject *parent)
    : QObject(parent)
{
    m_cameraReader = new QWindowsCameraReader(this);
    m_cameraExposure = new QWindowsCameraExposure(this);
    m_cameraFocus = new QWindowsCameraFocus(this);
    m_cameraImageProcessing = new QWindowsCameraImageProcessing(this);
    connect(m_cameraReader, SIGNAL(streamStarted()), this, SLOT(handleStreamStarted()));
    connect(m_cameraReader, SIGNAL(streamStopped()), this, SLOT(handleStreamStopped()));
}

QWindowsCameraSession::~QWindowsCameraSession() = default;

bool QWindowsCameraSession::isActive() const
{
    return m_active;
}

void QWindowsCameraSession::setActive(bool active)
{
    if (m_active == active)
        return;

    if (active) {
        m_cameraReader->start(QString::fromUtf8(m_activeCameraInfo.id()));
    } else {
        m_cameraReader->stop();
        m_active = false;
        emit activeChanged(m_active);
    }
}

void QWindowsCameraSession::setActiveCamera(const QCameraInfo &info)
{
    m_activeCameraInfo = info;
}

QImageEncoderSettings QWindowsCameraSession::imageSettings() const
{
    return m_imageEncoderSettings;
}

void QWindowsCameraSession::setImageSettings(const QImageEncoderSettings &settings)
{
    m_imageEncoderSettings = settings;
}

bool QWindowsCameraSession::isReadyForCapture() const
{
    return m_active && m_readyForCapture;
}

void QWindowsCameraSession::setReadyForCapture(bool ready)
{
    if (m_readyForCapture == ready)
        return;

    // Still image capture not yet supported
    // m_readyForCapture = ready;
    // emit readyForCaptureChanged(ready);
}

int QWindowsCameraSession::capture(const QString &fileName)
{
    return 0;
}

void QWindowsCameraSession::setVideoSink(QVideoSink *surface)
{
    m_cameraReader->setSurface(surface);
}

QWindowsCameraFocus *QWindowsCameraSession::focusControl()
{
    return m_cameraFocus;
}

QWindowsCameraExposure *QWindowsCameraSession::exposureControl()
{
    return m_cameraExposure;
}

QWindowsCameraImageProcessing *QWindowsCameraSession::imageProcessingControl()
{
    return m_cameraImageProcessing;
}

void QWindowsCameraSession::handleStreamStarted()
{
    m_active = true;
    emit activeChanged(m_active);
    setReadyForCapture(true);
}

void QWindowsCameraSession::handleStreamStopped()
{
    setReadyForCapture(false);
    m_active = false;
    emit activeChanged(m_active);
}

QT_END_NAMESPACE
