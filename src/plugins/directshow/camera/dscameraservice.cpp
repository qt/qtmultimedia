/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
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

#include <QtCore/qvariant.h>
#include <QtCore/qdebug.h>

#include "dscameraservice.h"
#include "dscameracontrol.h"
#include "dscamerasession.h"
#include "dsvideorenderer.h"
#include "dsvideodevicecontrol.h"
#include "dsimagecapturecontrol.h"
#include "dscameraviewfindersettingscontrol.h"

QT_BEGIN_NAMESPACE

DSCameraService::DSCameraService(QObject *parent):
    QMediaService(parent)
  , m_videoRenderer(0)
{
    m_session = new DSCameraSession(this);
    m_control = new DSCameraControl(m_session);
    m_videoDevice = new DSVideoDeviceControl(m_session);
    m_imageCapture = new DSImageCaptureControl(m_session);
    m_viewfinderSettings = new DSCameraViewfinderSettingsControl(m_session);
}

DSCameraService::~DSCameraService()
{
    delete m_control;
    delete m_viewfinderSettings;
    delete m_videoDevice;
    delete m_videoRenderer;
    delete m_imageCapture;
    delete m_session;
}

QMediaControl* DSCameraService::requestControl(const char *name)
{
    if(qstrcmp(name,QCameraControl_iid) == 0)
        return m_control;

    if (qstrcmp(name, QCameraImageCaptureControl_iid) == 0)
        return m_imageCapture;

    if (qstrcmp(name,QVideoRendererControl_iid) == 0) {
        if (!m_videoRenderer) {
            m_videoRenderer = new DSVideoRendererControl(m_session, this);
            return m_videoRenderer;
        }
    }

    if (qstrcmp(name,QVideoDeviceSelectorControl_iid) == 0)
        return m_videoDevice;

    if (qstrcmp(name, QCameraViewfinderSettingsControl2_iid) == 0)
        return m_viewfinderSettings;

    return 0;
}

void DSCameraService::releaseControl(QMediaControl *control)
{
    if (control == m_videoRenderer) {
        delete m_videoRenderer;
        m_videoRenderer = 0;
        return;
    }
}

QT_END_NAMESPACE
