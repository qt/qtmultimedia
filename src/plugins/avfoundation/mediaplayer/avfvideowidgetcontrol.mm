/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd and/or its subsidiary(-ies).
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

#include "avfvideowidgetcontrol.h"
#include "avfvideowidget.h"

#ifdef QT_DEBUG_AVF
#include <QtCore/QDebug>
#endif

#import <AVFoundation/AVFoundation.h>

QT_USE_NAMESPACE

AVFVideoWidgetControl::AVFVideoWidgetControl(QObject *parent)
    : QVideoWidgetControl(parent)
    , m_fullscreen(false)
    , m_brightness(0)
    , m_contrast(0)
    , m_hue(0)
    , m_saturation(0)
{
    m_videoWidget = new AVFVideoWidget(nullptr);
}

AVFVideoWidgetControl::~AVFVideoWidgetControl()
{
#ifdef QT_DEBUG_AVF
    qDebug() << Q_FUNC_INFO;
#endif
    delete m_videoWidget;
}

void AVFVideoWidgetControl::setLayer(void *playerLayer)
{
#ifdef QT_DEBUG_AVF
    qDebug() << Q_FUNC_INFO << playerLayer;
#endif

    m_videoWidget->setPlayerLayer(static_cast<AVPlayerLayer*>(playerLayer));

}

QWidget *AVFVideoWidgetControl::videoWidget()
{
    return m_videoWidget;
}

bool AVFVideoWidgetControl::isFullScreen() const
{
    return m_fullscreen;
}

void AVFVideoWidgetControl::setFullScreen(bool fullScreen)
{
    m_fullscreen = fullScreen;
}

Qt::AspectRatioMode AVFVideoWidgetControl::aspectRatioMode() const
{
    return m_videoWidget->aspectRatioMode();
}

void AVFVideoWidgetControl::setAspectRatioMode(Qt::AspectRatioMode mode)
{
    m_videoWidget->setAspectRatioMode(mode);
}

int AVFVideoWidgetControl::brightness() const
{
    return m_brightness;
}

void AVFVideoWidgetControl::setBrightness(int brightness)
{
    m_brightness = brightness;
}

int AVFVideoWidgetControl::contrast() const
{
    return m_contrast;
}

void AVFVideoWidgetControl::setContrast(int contrast)
{
    m_contrast = contrast;
}

int AVFVideoWidgetControl::hue() const
{
    return m_hue;
}

void AVFVideoWidgetControl::setHue(int hue)
{
    m_hue = hue;
}

int AVFVideoWidgetControl::saturation() const
{
    return m_saturation;
}

void AVFVideoWidgetControl::setSaturation(int saturation)
{
    m_saturation = saturation;
}

#include "moc_avfvideowidgetcontrol.cpp"
