/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Mobility Components.
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

#ifndef MFVIDEORENDERERCONTROL_H
#define MFVIDEORENDERERCONTROL_H

#include "qvideorenderercontrol.h"
#include <mfapi.h>
#include <mfidl.h>

QT_USE_NAMESPACE

class EVRCustomPresenterActivate;

class MFVideoRendererControl : public QVideoRendererControl
{
    Q_OBJECT
public:
    MFVideoRendererControl(QObject *parent = 0);
    ~MFVideoRendererControl();

    QAbstractVideoSurface *surface() const;
    void setSurface(QAbstractVideoSurface *surface);

    IMFActivate* createActivate();
    void releaseActivate();

protected:
    void customEvent(QEvent *event);

private Q_SLOTS:
    void supportedFormatsChanged();
    void present();

private:
    void clear();

    QAbstractVideoSurface *m_surface;
    IMFActivate *m_currentActivate;
    IMFSampleGrabberSinkCallback *m_callback;

    EVRCustomPresenterActivate *m_presenterActivate;
};

#endif
