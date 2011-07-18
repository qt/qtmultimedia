/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore/qvariant.h>
#include <QtCore/qdebug.h>
#include <QtGui/qwidget.h>
#include <QVideoWidgetControl.h>


#include "dscameraservice.h"
#include "dscameracontrol.h"
#include "dscamerasession.h"
#include "dsvideorenderer.h"
#include "dsvideodevicecontrol.h"
#include "dsimagecapturecontrol.h"
#include "dsvideowidgetcontrol.h"

QT_BEGIN_NAMESPACE

DSCameraService::DSCameraService(QObject *parent):
    QMediaService(parent)
{
    m_session = new DSCameraSession(this);

    m_control = new DSCameraControl(m_session);

    m_videoDevice = new DSVideoDeviceControl(m_session);

    m_videoRenderer = new DSVideoRendererControl(m_session, this);

    m_imageCapture = new DSImageCaptureControl(m_session);

    m_viewFinderWidget = new DSVideoWidgetControl(m_session);

    m_device = QByteArray("default");
}

DSCameraService::~DSCameraService()
{
    delete m_control;
    delete m_videoDevice;
    delete m_videoRenderer;
    delete m_imageCapture;
    delete m_viewFinderWidget;
    delete m_session;
}

QMediaControl* DSCameraService::requestControl(const char *name)
{
    if(qstrcmp(name,QCameraControl_iid) == 0)
        return m_control;

    if (qstrcmp(name, QCameraImageCaptureControl_iid) == 0)
        return m_imageCapture;

    if (qstrcmp(name, QVideoWidgetControl_iid) == 0) {
        if (m_viewFinderWidget) {
            return m_viewFinderWidget;
        }
    }

    if(qstrcmp(name,QVideoRendererControl_iid) == 0)
        return m_videoRenderer;

    if(qstrcmp(name,QVideoDeviceControl_iid) == 0)
        return m_videoDevice;

    return 0;
}

void DSCameraService::releaseControl(QMediaControl *control)
{
   // Implemented as a singleton, so we do nothing.
}

QT_END_NAMESPACE
