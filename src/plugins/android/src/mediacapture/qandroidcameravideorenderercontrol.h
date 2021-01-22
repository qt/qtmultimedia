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

#ifndef QANDROIDCAMERAVIDEORENDERERCONTROL_H
#define QANDROIDCAMERAVIDEORENDERERCONTROL_H

#include <qvideorenderercontrol.h>

QT_BEGIN_NAMESPACE

class QAndroidCameraSession;
class QAndroidTextureVideoOutput;
class QAndroidCameraDataVideoOutput;

class QAndroidCameraVideoRendererControl : public QVideoRendererControl
{
    Q_OBJECT
public:
    QAndroidCameraVideoRendererControl(QAndroidCameraSession *session, QObject *parent = 0);
    ~QAndroidCameraVideoRendererControl() override;

    QAbstractVideoSurface *surface() const override;
    void setSurface(QAbstractVideoSurface *surface) override;

    QAndroidCameraSession *cameraSession() const { return m_cameraSession; }

private:
    QAndroidCameraSession *m_cameraSession;
    QAbstractVideoSurface *m_surface;
    QAndroidTextureVideoOutput *m_textureOutput;
    QAndroidCameraDataVideoOutput *m_dataOutput;
};

QT_END_NAMESPACE

#endif // QANDROIDCAMERAVIDEORENDERERCONTROL_H
