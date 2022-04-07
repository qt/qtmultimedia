/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Mobility Components.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
#include <private/qwindowsmfdefs_p.h>

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
