/****************************************************************************
**
** Copyright (C) 2016 Research In Motion
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
#ifndef MMRENDERERVIDEOWINDOWCONTROL_H
#define MMRENDERERVIDEOWINDOWCONTROL_H

#include "mmrenderermetadata.h"
#include <qvideowindowcontrol.h>
#include <screen/screen.h>

typedef struct mmr_context mmr_context_t;

QT_BEGIN_NAMESPACE

class MmRendererVideoWindowControl : public QVideoWindowControl
{
    Q_OBJECT
public:
    explicit MmRendererVideoWindowControl(QObject *parent = 0);
    ~MmRendererVideoWindowControl();

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

    //
    // Called by media control
    //
    void detachDisplay();
    void attachDisplay(mmr_context_t *context);
    void setMetaData(const MmRendererMetaData &metaData);
    void screenEventHandler(const screen_event_t &event);

private:
    QWindow *findWindow(WId id) const;
    void updateVideoPosition();
    void updateBrightness();
    void updateContrast();
    void updateHue();
    void updateSaturation();

    int m_videoId;
    WId m_winId;
    QRect m_displayRect;
    mmr_context_t *m_context;
    bool m_fullscreen;
    MmRendererMetaData m_metaData;
    Qt::AspectRatioMode m_aspectRatioMode;
    QString m_windowName;
    screen_window_t m_window;
    int m_hue;
    int m_brightness;
    int m_contrast;
    int m_saturation;
};

QT_END_NAMESPACE

#endif
