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

#ifndef EVRVIDEOWINDOWCONTROL_H
#define EVRVIDEOWINDOWCONTROL_H

#include "qvideowindowcontrol.h"

#include "evrdefs.h"

QT_BEGIN_NAMESPACE

class EvrVideoWindowControl : public QVideoWindowControl
{
    Q_OBJECT
public:
    EvrVideoWindowControl(QObject *parent = 0);
    ~EvrVideoWindowControl() override;

    bool setEvr(IUnknown *evr);

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

    void applyImageControls();

private:
    void clear();
    DXVA2_Fixed32 scaleProcAmpValue(DWORD prop, int value) const;

    WId m_windowId;
    COLORREF m_windowColor;
    DWORD m_dirtyValues;
    Qt::AspectRatioMode m_aspectRatioMode;
    QRect m_displayRect;
    int m_brightness;
    int m_contrast;
    int m_hue;
    int m_saturation;
    bool m_fullScreen;

    IMFVideoDisplayControl *m_displayControl;
    IMFVideoProcessor *m_processor;
};

QT_END_NAMESPACE

#endif
