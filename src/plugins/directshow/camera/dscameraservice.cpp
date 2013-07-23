/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore/qvariant.h>
#include <QtCore/qdebug.h>

#if defined(HAVE_WIDGETS)
#include <QtWidgets/qwidget.h>
#include <QVideoWidgetControl>
#endif

#include "dscameraservice.h"
#include "dscameracontrol.h"
#include "dscamerasession.h"
#include "dsvideorenderer.h"
#include "dsvideodevicecontrol.h"
#include "dsimagecapturecontrol.h"

#if defined(HAVE_WIDGETS)
#include "dsvideowidgetcontrol.h"
#endif

QT_BEGIN_NAMESPACE

DSCameraService::DSCameraService(QObject *parent):
    QMediaService(parent)
#if defined(HAVE_WIDGETS)
  , m_viewFinderWidget(0)
  #endif
  , m_videoRenderer(0)
{
    m_session = new DSCameraSession(this);

    m_control = new DSCameraControl(m_session);

    m_videoDevice = new DSVideoDeviceControl(m_session);

    m_imageCapture = new DSImageCaptureControl(m_session);

    m_device = QByteArray("default");
}

DSCameraService::~DSCameraService()
{
    delete m_control;
    delete m_videoDevice;
    delete m_videoRenderer;
    delete m_imageCapture;
#if defined(HAVE_WIDGETS)
    delete m_viewFinderWidget;
#endif
    delete m_session;
}

QMediaControl* DSCameraService::requestControl(const char *name)
{
    if(qstrcmp(name,QCameraControl_iid) == 0)
        return m_control;

    if (qstrcmp(name, QCameraImageCaptureControl_iid) == 0)
        return m_imageCapture;

#if defined(HAVE_WIDGETS)
    if (qstrcmp(name, QVideoWidgetControl_iid) == 0) {
        if (!m_viewFinderWidget && !m_videoRenderer) {
            m_viewFinderWidget = new DSVideoWidgetControl(m_session);
            return m_viewFinderWidget;
        }
    }
#endif

    if (qstrcmp(name,QVideoRendererControl_iid) == 0) {
#if defined(HAVE_WIDGETS)
        if (!m_videoRenderer && !m_viewFinderWidget) {
#else
        if (!m_videoRenderer) {
#endif
            m_videoRenderer = new DSVideoRendererControl(m_session, this);
            return m_videoRenderer;
        }
    }

    if (qstrcmp(name,QVideoDeviceSelectorControl_iid) == 0)
        return m_videoDevice;

    return 0;
}

void DSCameraService::releaseControl(QMediaControl *control)
{
    if (control == m_videoRenderer) {
        delete m_videoRenderer;
        m_videoRenderer = 0;
        return;
    }

#if defined(HAVE_WIDGETS)
    if (control == m_viewFinderWidget) {
        delete m_viewFinderWidget;
        m_viewFinderWidget = 0;
        return;
    }
#endif
}

QT_END_NAMESPACE
