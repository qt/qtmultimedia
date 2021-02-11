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

#ifndef QVIDEOWIDGET_P_H
#define QVIDEOWIDGET_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <qtmultimediawidgetdefs.h>
#include "qvideowidget.h"

#ifndef QT_NO_OPENGL
#include <QOpenGLWidget>
#endif

#include "qpaintervideosurface_p.h"

#include <QtCore/qpointer.h>

QT_BEGIN_NAMESPACE

class QVideoRendererControl;

class QRendererVideoWidgetBackend : public QObject
{
    Q_OBJECT
public:
    QRendererVideoWidgetBackend(QWidget *widget);
    ~QRendererVideoWidgetBackend();

    QAbstractVideoSurface *videoSurface() const;

    void releaseControl();
    void clearSurface();

    void setBrightness(int brightness);
    void setContrast(int contrast);
    void setHue(int hue);
    void setSaturation(int saturation);

    void setFullScreen(bool fullScreen);

    Qt::AspectRatioMode aspectRatioMode() const;
    void setAspectRatioMode(Qt::AspectRatioMode mode);

    QSize sizeHint() const;

    void showEvent();
    void hideEvent(QHideEvent *event);
    void resizeEvent(QResizeEvent *event);
    void moveEvent(QMoveEvent *event);
    void paintEvent(QPaintEvent *event);

Q_SIGNALS:
    void fullScreenChanged(bool fullScreen);
    void brightnessChanged(int brightness);
    void contrastChanged(int contrast);
    void hueChanged(int hue);
    void saturationChanged(int saturation);

private Q_SLOTS:
    void formatChanged(const QVideoSurfaceFormat &format);
    void frameChanged();

private:
    void updateRects();

    QVideoRendererControl *m_rendererControl;
    QWidget *m_widget;
    QPainterVideoSurface *m_surface;
    Qt::AspectRatioMode m_aspectRatioMode;
    QRect m_boundingRect;
    QRectF m_sourceRect;
    QSize m_nativeSize;
    bool m_updatePaintDevice;
};

class QVideoOutputControl;

class QVideoWidgetPrivate
{
    Q_DECLARE_PUBLIC(QVideoWidget)
public:
    QVideoWidget *q_ptr = nullptr;
    QRendererVideoWidgetBackend *backend = nullptr;
    int brightness = 0;
    int contrast = 0;
    int hue = 0;
    int saturation = 0;
    Qt::AspectRatioMode aspectRatioMode = Qt::KeepAspectRatio;
    Qt::WindowFlags nonFullScreenFlags;
    bool wasFullScreen = false;

    bool createBackend();

    void _q_brightnessChanged(int brightness);
    void _q_contrastChanged(int contrast);
    void _q_hueChanged(int hue);
    void _q_saturationChanged(int saturation);
    void _q_fullScreenChanged(bool fullScreen);
    void _q_dimensionsChanged();
};

QT_END_NAMESPACE


#endif
