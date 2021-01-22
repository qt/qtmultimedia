/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#include "qvideosurfaceoutput_p.h"

#include <qabstractvideosurface.h>
#include <qmediaservice.h>
#include <qvideorenderercontrol.h>

/*!
 * \class QVideoSurfaceOutput
 * \internal
 */
QVideoSurfaceOutput::QVideoSurfaceOutput(QObject*parent)
    :  QObject(parent)
{
}

QVideoSurfaceOutput::~QVideoSurfaceOutput()
{
    if (m_control) {
        m_control.data()->setSurface(nullptr);
        m_service.data()->releaseControl(m_control.data());
    }
}

QMediaObject *QVideoSurfaceOutput::mediaObject() const
{
    return m_object.data();
}

void QVideoSurfaceOutput::setVideoSurface(QAbstractVideoSurface *surface)
{
    m_surface = surface;

    if (m_control)
        m_control.data()->setSurface(surface);
}

bool QVideoSurfaceOutput::setMediaObject(QMediaObject *object)
{
    if (m_control) {
        m_control.data()->setSurface(nullptr);
        m_service.data()->releaseControl(m_control.data());
    }
    m_control.clear();
    m_service.clear();
    m_object.clear();

    if (object) {
        if (QMediaService *service = object->service()) {
            if (QMediaControl *control = service->requestControl(QVideoRendererControl_iid)) {
                if ((m_control = qobject_cast<QVideoRendererControl *>(control))) {
                    m_service = service;
                    m_object = object;
                    m_control.data()->setSurface(m_surface.data());

                    return true;
                }
                service->releaseControl(control);
            }
        }
    }
    return false;
}
