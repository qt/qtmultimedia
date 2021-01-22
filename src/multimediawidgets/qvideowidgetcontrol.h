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

#ifndef QVIDEOWIDGETCONTROL_H
#define QVIDEOWIDGETCONTROL_H

#include <QtMultimediaWidgets/qvideowidget.h>
#include <QtMultimedia/qmediacontrol.h>

#include <QtWidgets/qwidget.h>

QT_BEGIN_NAMESPACE


class QVideoWidgetControlPrivate;

class Q_MULTIMEDIAWIDGETS_EXPORT QVideoWidgetControl : public QMediaControl
{
    Q_OBJECT

public:
    virtual ~QVideoWidgetControl();

    virtual QWidget *videoWidget() = 0;

    virtual Qt::AspectRatioMode aspectRatioMode() const = 0;
    virtual void setAspectRatioMode(Qt::AspectRatioMode mode) = 0;

    virtual bool isFullScreen() const = 0;
    virtual void setFullScreen(bool fullScreen) = 0;

    virtual int brightness() const = 0;
    virtual void setBrightness(int brightness) = 0;

    virtual int contrast() const = 0;
    virtual void setContrast(int contrast) = 0;

    virtual int hue() const = 0;
    virtual void setHue(int hue) = 0;

    virtual int saturation() const = 0;
    virtual void setSaturation(int saturation) = 0;

Q_SIGNALS:
    void fullScreenChanged(bool fullScreen);
    void brightnessChanged(int brightness);
    void contrastChanged(int contrast);
    void hueChanged(int hue);
    void saturationChanged(int saturation);

protected:
    explicit QVideoWidgetControl(QObject *parent = nullptr);
};

#define QVideoWidgetControl_iid "org.qt-project.qt.videowidgetcontrol/5.0"
Q_MEDIA_DECLARE_CONTROL(QVideoWidgetControl, QVideoWidgetControl_iid)

QT_END_NAMESPACE


#endif
