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

#ifndef QANDROIDMEDIAPLAYERVIDEORENDERERCONTROL_H
#define QANDROIDMEDIAPLAYERVIDEORENDERERCONTROL_H

#include <qvideorenderercontrol.h>

QT_BEGIN_NAMESPACE

class QAndroidMediaPlayerControl;
class QAndroidTextureVideoOutput;

class QAndroidMediaPlayerVideoRendererControl : public QVideoRendererControl
{
    Q_OBJECT
public:
    QAndroidMediaPlayerVideoRendererControl(QAndroidMediaPlayerControl *mediaPlayer, QObject *parent = 0);
    ~QAndroidMediaPlayerVideoRendererControl() override;

    QAbstractVideoSurface *surface() const override;
    void setSurface(QAbstractVideoSurface *surface) override;

private:
    QAndroidMediaPlayerControl *m_mediaPlayerControl;
    QAbstractVideoSurface *m_surface;
    QAndroidTextureVideoOutput *m_textureOutput;
};

QT_END_NAMESPACE

#endif // QANDROIDMEDIAPLAYERVIDEORENDERERCONTROL_H
