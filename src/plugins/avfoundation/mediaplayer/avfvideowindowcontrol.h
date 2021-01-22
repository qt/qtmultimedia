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

#ifndef AVFVIDEOWINDOWCONTROL_H
#define AVFVIDEOWINDOWCONTROL_H

#include <QVideoWindowControl>

@class AVPlayerLayer;
#if defined(Q_OS_OSX)
@class NSView;
typedef NSView NativeView;
#else
@class UIView;
typedef UIView NativeView;
#endif

#include "avfvideooutput.h"

QT_BEGIN_NAMESPACE

class AVFVideoWindowControl : public QVideoWindowControl, public AVFVideoOutput
{
    Q_OBJECT
    Q_INTERFACES(AVFVideoOutput)

public:
    AVFVideoWindowControl(QObject *parent = nullptr);
    virtual ~AVFVideoWindowControl();

    // QVideoWindowControl interface
public:
    WId winId() const override;
    void setWinId(WId id) override;

    QRect displayRect() const override;
    void setDisplayRect(const QRect &rect) override;

    bool isFullScreen() const override;
    void setFullScreen(bool fullScreen) override;

    void repaint() override;
    QSize nativeSize() const override;

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

    // AVFVideoOutput interface
    void setLayer(void *playerLayer) override;

private:
    void updateAspectRatio();
    void updatePlayerLayerBounds();

    WId m_winId;
    QRect m_displayRect;
    bool m_fullscreen;
    int m_brightness;
    int m_contrast;
    int m_hue;
    int m_saturation;
    Qt::AspectRatioMode m_aspectRatioMode;
    QSize m_nativeSize;
    AVPlayerLayer *m_playerLayer;
    NativeView *m_nativeView;
};

QT_END_NAMESPACE

#endif // AVFVIDEOWINDOWCONTROL_H
