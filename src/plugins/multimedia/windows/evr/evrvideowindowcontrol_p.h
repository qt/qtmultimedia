// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef EVRVIDEOWINDOWCONTROL_H
#define EVRVIDEOWINDOWCONTROL_H

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

#include <d3d9.h>
#include <dxva2api.h>
#include <evr9.h>
#include <evr.h>
#include <private/qplatformvideosink_p.h>

QT_BEGIN_NAMESPACE

class EvrVideoWindowControl : public QPlatformVideoSink
{
    Q_OBJECT
public:
    EvrVideoWindowControl(QVideoSink *parent = 0);
    ~EvrVideoWindowControl() override;

    bool setEvr(IUnknown *evr);

    void setWinId(WId id) override;

    void setDisplayRect(const QRect &rect) override;

    void setFullScreen(bool fullScreen) override;

    void setAspectRatioMode(Qt::AspectRatioMode mode) override;

    void setBrightness(float brightness) override;
    void setContrast(float contrast) override;
    void setHue(float hue) override;
    void setSaturation(float saturation) override;

    void applyImageControls();

private:
    void clear();
    DXVA2_Fixed32 scaleProcAmpValue(DWORD prop, float value) const;

    WId m_windowId;
    COLORREF m_windowColor;
    DWORD m_dirtyValues;
    Qt::AspectRatioMode m_aspectRatioMode;
    QRect m_displayRect;
    float m_brightness;
    float m_contrast;
    float m_hue;
    float m_saturation;
    bool m_fullScreen;

    IMFVideoDisplayControl *m_displayControl;
    IMFVideoProcessor *m_processor;
};

QT_END_NAMESPACE

#endif
