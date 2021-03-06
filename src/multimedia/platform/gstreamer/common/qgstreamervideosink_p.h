/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
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

#ifndef QGSTREAMERVIDEOWINDOW_H
#define QGSTREAMERVIDEOWINDOW_H

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

#include <private/qtmultimediaglobal_p.h>
#include <private/qplatformvideosink_p.h>

#include <private/qgstpipeline_p.h>
#include <private/qgstreamervideooverlay_p.h>
#include <QtGui/qcolor.h>
#include <qvideosink.h>

QT_BEGIN_NAMESPACE
class QGstreamerVideoRenderer;

class Q_MULTIMEDIA_EXPORT QGstreamerVideoSink
    : public QPlatformVideoSink
{
    Q_OBJECT
public:
    explicit QGstreamerVideoSink(QVideoSink *parent = 0);
    ~QGstreamerVideoSink();

    void setWinId(WId id) override;

    void setRhi(QRhi *rhi) override;

    void setDisplayRect(const QRect &rect) override;

    void setFullScreen(bool fullScreen) override;

    QSize nativeSize() const override;

    void setAspectRatioMode(Qt::AspectRatioMode mode) override;

    void setBrightness(float brightness) override;
    void setContrast(float contrast) override;
    void setHue(float hue) override;
    void setSaturation(float saturation) override;

    QGstElement gstSink();

    bool isReady() const { return m_windowId != 0; }

    void setPipeline(QGstPipeline pipeline);

private:
    void createOverlay();
    void createRenderer();
    void updateSinkElement();

    QGstPipeline gstPipeline;
    QGstBin sinkBin;
    QGstElement gstPreprocess;
    QGstElement gstVideoSink;

    QGstreamerVideoOverlay *m_videoOverlay = nullptr;
    QGstreamerVideoRenderer *m_videoRenderer = nullptr;
    WId m_windowId = 0;
    QRhi *m_rhi = nullptr;
    QRect m_displayRect;
    bool m_fullScreen = false;
    mutable QColor m_colorKey = QColor::Invalid;
};

QT_END_NAMESPACE

#endif
