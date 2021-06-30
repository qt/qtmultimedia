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

#include "qandroidcameravideorenderercontrol_p.h"

#include "qandroidcamerasession_p.h"
#include "qandroidvideooutput_p.h"
#include "androidsurfaceview_p.h"
#include "qandroidmultimediautils_p.h"
#include <private/qrhi_p.h>
#include <qvideosink.h>
#include <qvideoframeformat.h>
#include <qcoreapplication.h>
#include <qthread.h>

QT_BEGIN_NAMESPACE

QAndroidCameraVideoRendererControl::QAndroidCameraVideoRendererControl(QAndroidCameraSession *session, QObject *parent)
    : QObject(parent)
    , m_cameraSession(session)
    , m_surface(0)
    , m_textureOutput(0)
{
}

QAndroidCameraVideoRendererControl::~QAndroidCameraVideoRendererControl()
{
    m_cameraSession->setVideoOutput(0);
}

QVideoSink *QAndroidCameraVideoRendererControl::surface() const
{
    return m_surface;
}

void QAndroidCameraVideoRendererControl::setSurface(QVideoSink *surface)
{
    if (m_surface == surface)
        return;

    m_surface = surface;
    delete m_textureOutput;
    m_textureOutput = nullptr;

    if (m_surface) {
        m_textureOutput = new QAndroidTextureVideoOutput(this);
        m_textureOutput->setSurface(m_surface);
    }

    m_cameraSession->setVideoOutput(m_textureOutput);
}

QT_END_NAMESPACE

