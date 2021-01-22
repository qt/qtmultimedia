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

#ifndef QVIDEOWIDGET_H
#define QVIDEOWIDGET_H

#include <QtWidgets/qwidget.h>

#include <QtMultimediaWidgets/qtmultimediawidgetdefs.h>
#include <QtMultimedia/qmediabindableinterface.h>

QT_BEGIN_NAMESPACE


class QMediaObject;

class QVideoWidgetPrivate;
class QAbstractVideoSurface;
class Q_MULTIMEDIAWIDGETS_EXPORT QVideoWidget : public QWidget, public QMediaBindableInterface
{
    Q_OBJECT
    Q_INTERFACES(QMediaBindableInterface)
    Q_PROPERTY(QMediaObject* mediaObject READ mediaObject WRITE setMediaObject)
    Q_PROPERTY(bool fullScreen READ isFullScreen WRITE setFullScreen NOTIFY fullScreenChanged)
    Q_PROPERTY(Qt::AspectRatioMode aspectRatioMode READ aspectRatioMode WRITE setAspectRatioMode)
    Q_PROPERTY(int brightness READ brightness WRITE setBrightness NOTIFY brightnessChanged)
    Q_PROPERTY(int contrast READ contrast WRITE setContrast NOTIFY contrastChanged)
    Q_PROPERTY(int hue READ hue WRITE setHue NOTIFY hueChanged)
    Q_PROPERTY(int saturation READ saturation WRITE setSaturation NOTIFY saturationChanged)
    Q_PROPERTY(QAbstractVideoSurface* videoSurface READ videoSurface CONSTANT)

public:
    explicit QVideoWidget(QWidget *parent = nullptr);
    ~QVideoWidget();

    QMediaObject *mediaObject() const override;
    QAbstractVideoSurface *videoSurface() const;

#ifdef Q_QDOC
    bool isFullScreen() const;
#endif

    Qt::AspectRatioMode aspectRatioMode() const;

    int brightness() const;
    int contrast() const;
    int hue() const;
    int saturation() const;

    QSize sizeHint() const override;
#if defined(Q_OS_WIN)
#  if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;
#  else
    bool nativeEvent(const QByteArray &eventType, void *message, long *result) override;
#  endif
#endif

public Q_SLOTS:
    void setFullScreen(bool fullScreen);
    void setAspectRatioMode(Qt::AspectRatioMode mode);
    void setBrightness(int brightness);
    void setContrast(int contrast);
    void setHue(int hue);
    void setSaturation(int saturation);

Q_SIGNALS:
    void fullScreenChanged(bool fullScreen);
    void brightnessChanged(int brightness);
    void contrastChanged(int contrast);
    void hueChanged(int hue);
    void saturationChanged(int saturation);

protected:
    bool event(QEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void moveEvent(QMoveEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

    bool setMediaObject(QMediaObject *object) override;

    QVideoWidget(QVideoWidgetPrivate &dd, QWidget *parent);
    QVideoWidgetPrivate *d_ptr;

private:
    Q_DECLARE_PRIVATE(QVideoWidget)
    Q_PRIVATE_SLOT(d_func(), void _q_serviceDestroyed())
    Q_PRIVATE_SLOT(d_func(), void _q_brightnessChanged(int))
    Q_PRIVATE_SLOT(d_func(), void _q_contrastChanged(int))
    Q_PRIVATE_SLOT(d_func(), void _q_hueChanged(int))
    Q_PRIVATE_SLOT(d_func(), void _q_saturationChanged(int))
    Q_PRIVATE_SLOT(d_func(), void _q_fullScreenChanged(bool))
    Q_PRIVATE_SLOT(d_func(), void _q_dimensionsChanged())
};

QT_END_NAMESPACE


#endif
