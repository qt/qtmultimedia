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

#ifndef DSVIDEORENDERER_H
#define DSVIDEORENDERER_H

#include <qvideorenderercontrol.h>
#include "dscamerasession.h"

QT_BEGIN_NAMESPACE


class DSVideoRendererControl : public QVideoRendererControl
{
    Q_OBJECT
public:
    DSVideoRendererControl(DSCameraSession* session, QObject *parent = nullptr);
    ~DSVideoRendererControl() override;

    QAbstractVideoSurface *surface() const override;
    void setSurface(QAbstractVideoSurface *surface) override;

    void setSession(DSCameraSession* session);

private:
    QAbstractVideoSurface* m_surface = nullptr;
    DSCameraSession* m_session;
};

QT_END_NAMESPACE

#endif // DSVIDEORENDERER_H
