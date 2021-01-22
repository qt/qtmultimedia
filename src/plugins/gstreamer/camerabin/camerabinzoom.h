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

#ifndef CAMERABINZOOMCONTROL_H
#define CAMERABINZOOMCONTROL_H

#include <qcamerazoomcontrol.h>
#include <gst/gst.h>

QT_BEGIN_NAMESPACE

class CameraBinSession;

class CameraBinZoom  : public QCameraZoomControl
{
    Q_OBJECT
public:
    CameraBinZoom(CameraBinSession *session);
    virtual ~CameraBinZoom();

    qreal maximumOpticalZoom() const override;
    qreal maximumDigitalZoom() const override;

    qreal requestedOpticalZoom() const override;
    qreal requestedDigitalZoom() const override;
    qreal currentOpticalZoom() const override;
    qreal currentDigitalZoom() const override;

    void zoomTo(qreal optical, qreal digital) override;

private:
    static void updateZoom(GObject *o, GParamSpec *p, gpointer d);
    static void updateMaxZoom(GObject *o, GParamSpec *p, gpointer d);

    CameraBinSession *m_session;
    qreal m_requestedOpticalZoom;
    qreal m_requestedDigitalZoom;
};

QT_END_NAMESPACE

#endif // CAMERABINZOOMCONTROL_H
