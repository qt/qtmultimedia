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


class QMediaService;

class QVideoWidgetControlInterface
{
public:
    virtual ~QVideoWidgetControlInterface() {}

    virtual void setBrightness(int brightness) = 0;
    virtual void setContrast(int contrast) = 0;
    virtual void setHue(int hue) = 0;
    virtual void setSaturation(int saturation) = 0;

    virtual void setFullScreen(bool fullScreen) = 0;

    virtual Qt::AspectRatioMode aspectRatioMode() const = 0;
    virtual void setAspectRatioMode(Qt::AspectRatioMode mode) = 0;
};

class QVideoWidgetBackend : public QObject, public QVideoWidgetControlInterface
{
    Q_OBJECT
public:
    virtual QSize sizeHint() const = 0;

    virtual void showEvent() = 0;
    virtual void hideEvent(QHideEvent *event) = 0;
    virtual void resizeEvent(QResizeEvent *event) = 0;
    virtual void moveEvent(QMoveEvent *event) = 0;
    virtual void paintEvent(QPaintEvent *event) = 0;
};

class QVideoWidgetControl;

class QVideoWidgetControlBackend : public QObject, public QVideoWidgetControlInterface
{
    Q_OBJECT
public:
    QVideoWidgetControlBackend(QMediaService *service, QVideoWidgetControl *control, QWidget *widget);

    void releaseControl();

    void setBrightness(int brightness) override;
    void setContrast(int contrast) override;
    void setHue(int hue) override;
    void setSaturation(int saturation) override;

    void setFullScreen(bool fullScreen) override;

    Qt::AspectRatioMode aspectRatioMode() const override;
    void setAspectRatioMode(Qt::AspectRatioMode mode) override;

private:
    QMediaService *m_service;
    QVideoWidgetControl *m_widgetControl;
};


class QVideoRendererControl;

class QRendererVideoWidgetBackend : public QVideoWidgetBackend
{
    Q_OBJECT
public:
    QRendererVideoWidgetBackend(QMediaService *service, QVideoRendererControl *control, QWidget *widget);
    ~QRendererVideoWidgetBackend();

    QAbstractVideoSurface *videoSurface() const;

    void releaseControl();
    void clearSurface();

    void setBrightness(int brightness) override;
    void setContrast(int contrast) override;
    void setHue(int hue) override;
    void setSaturation(int saturation) override;

    void setFullScreen(bool fullScreen) override;

    Qt::AspectRatioMode aspectRatioMode() const override;
    void setAspectRatioMode(Qt::AspectRatioMode mode) override;

    QSize sizeHint() const override;

    void showEvent() override;
    void hideEvent(QHideEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void moveEvent(QMoveEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

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

    QMediaService *m_service;
    QVideoRendererControl *m_rendererControl;
    QWidget *m_widget;
    QPainterVideoSurface *m_surface;
    Qt::AspectRatioMode m_aspectRatioMode;
    QRect m_boundingRect;
    QRectF m_sourceRect;
    QSize m_nativeSize;
    bool m_updatePaintDevice;
};

class QVideoWindowControl;

class QWindowVideoWidgetBackend : public QVideoWidgetBackend
{
    Q_OBJECT
public:
    QWindowVideoWidgetBackend(QMediaService *service, QVideoWindowControl *control, QWidget *widget);
    ~QWindowVideoWidgetBackend();

    void releaseControl();

    void setBrightness(int brightness) override;
    void setContrast(int contrast) override;
    void setHue(int hue) override;
    void setSaturation(int saturation) override;

   void setFullScreen(bool fullScreen) override;

    Qt::AspectRatioMode aspectRatioMode() const override;
    void setAspectRatioMode(Qt::AspectRatioMode mode) override;

    QSize sizeHint() const override;

    void showEvent() override;
    void hideEvent(QHideEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void moveEvent(QMoveEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    void updateDisplayRect();

    QMediaService *m_service;
    QVideoWindowControl *m_windowControl;
    QWidget *m_widget;
    QSize m_pixelAspectRatio;
};

class QMediaService;
class QVideoOutputControl;

class QVideoWidgetPrivate
{
    Q_DECLARE_PUBLIC(QVideoWidget)
public:
    QVideoWidget *q_ptr = nullptr;
    QPointer<QMediaObject> mediaObject;
    QMediaService *service = nullptr;
    QVideoWidgetControlBackend *widgetBackend = nullptr;
    QWindowVideoWidgetBackend *windowBackend = nullptr;
    QRendererVideoWidgetBackend *rendererBackend = nullptr;
    QVideoWidgetControlInterface *currentControl = nullptr;
    QVideoWidgetBackend *currentBackend = nullptr;
    int brightness = 0;
    int contrast = 0;
    int hue = 0;
    int saturation = 0;
    Qt::AspectRatioMode aspectRatioMode = Qt::KeepAspectRatio;
    Qt::WindowFlags nonFullScreenFlags;
    bool wasFullScreen = false;

    bool createWidgetBackend();
    bool createWindowBackend();
    bool createRendererBackend();

    void setCurrentControl(QVideoWidgetControlInterface *control);
    void clearService();

    void _q_serviceDestroyed();
    void _q_brightnessChanged(int brightness);
    void _q_contrastChanged(int contrast);
    void _q_hueChanged(int hue);
    void _q_saturationChanged(int saturation);
    void _q_fullScreenChanged(bool fullScreen);
    void _q_dimensionsChanged();
};

QT_END_NAMESPACE


#endif
