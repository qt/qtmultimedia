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

#ifndef VMR9VIDEOWINDOWCONTROL_H
#define VMR9VIDEOWINDOWCONTROL_H

#include "qvideowindowcontrol.h"

#include <dshow.h>
#include <d3d9.h>
#include <vmr9.h>

QT_BEGIN_NAMESPACE

class Vmr9VideoWindowControl : public QVideoWindowControl
{
    Q_OBJECT
public:
    Vmr9VideoWindowControl(QObject *parent = nullptr);
    ~Vmr9VideoWindowControl() override;

    IBaseFilter *filter() const { return m_filter; }

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

private:
    void setProcAmpValues();
    float scaleProcAmpValue(
            IVMRMixerControl9 *control, VMR9ProcAmpControlFlags property, int value) const;

    IBaseFilter *m_filter;
    WId m_windowId = 0;
    COLORREF m_windowColor = RGB(0, 0, 0);
    DWORD m_dirtyValues = 0;
    Qt::AspectRatioMode m_aspectRatioMode = Qt::KeepAspectRatio;
    QRect m_displayRect;
    int m_brightness = 0;
    int m_contrast = 0;
    int m_hue = 0;
    int m_saturation = 0;
    bool m_fullScreen = false;
};

QT_END_NAMESPACE

#endif
