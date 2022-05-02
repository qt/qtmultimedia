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
******************************************************************************/
#ifndef MMRENDERERVIDEOWINDOWCONTROL_H
#define MMRENDERERVIDEOWINDOWCONTROL_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "mmrenderermetadata_p.h"
#include "private/qplatformvideosink_p.h"
#include <screen/screen.h>

typedef struct mmr_context mmr_context_t;

QT_BEGIN_NAMESPACE

class MmRendererVideoWindowControl : public QPlatformVideoSink
{
    Q_OBJECT
public:
    explicit MmRendererVideoWindowControl(QObject *parent = 0);
    ~MmRendererVideoWindowControl();

    WId winId() const override;
    void setWinId(WId id) override;

    void setDisplayRect(const QRect &rect) override;

    void setFullScreen(bool fullScreen) override;

    void repaint() override;

    Qt::AspectRatioMode aspectRatioMode() const override;
    void setAspectRatioMode(Qt::AspectRatioMode mode) override;

    void setBrightness(float brightness) override;
    void setContrast(float contrast) override;
    void setHue(float hue) override;
    void setSaturation(float saturation) override;

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
    float m_hue;
    float m_brightness;
    float m_contrast;
    float m_saturation;
};

QT_END_NAMESPACE

#endif
