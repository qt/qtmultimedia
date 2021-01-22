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

#ifndef AVFVIDEOWIDGETCONTROL_H
#define AVFVIDEOWIDGETCONTROL_H

#include <qvideowidgetcontrol.h>
#include "avfvideooutput.h"

@class AVPlayerLayer;

QT_BEGIN_NAMESPACE

class AVFVideoWidget;

class AVFVideoWidgetControl : public QVideoWidgetControl, public AVFVideoOutput
{
    Q_OBJECT
    Q_INTERFACES(AVFVideoOutput)
public:
    AVFVideoWidgetControl(QObject *parent = nullptr);
    virtual ~AVFVideoWidgetControl();

    void setLayer(void *playerLayer) override;

    QWidget *videoWidget() override;

    bool isFullScreen() const override;
    void setFullScreen(bool fullScreen) override;

    Qt::AspectRatioMode aspectRatioMode() const override;
    void setAspectRatioMode(Qt::AspectRatioMode mode) override;

    int brightness() const override;
    void setBrightness(int brightness) override;

    int contrast() const override;
    void setContrast(int contrast) override;

    int hue() const override;
    void setHue(int hue) override;

    int saturation() const override;
    void setSaturation(int saturation) override;

private:
    AVFVideoWidget *m_videoWidget;

    bool m_fullscreen;
    int m_brightness;
    int m_contrast;
    int m_hue;
    int m_saturation;

};

QT_END_NAMESPACE

#endif // AVFVIDEOWIDGETCONTROL_H
