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

#ifndef QVIDEOWINDOWCONTROL_H
#define QVIDEOWINDOWCONTROL_H

#include <QtMultimedia/qmediacontrol.h>
#include <QtCore/qrect.h>
#include <QtCore/qsize.h>
#include <QtGui/qwindowdefs.h>

QT_BEGIN_NAMESPACE

// Required for QDoc workaround
class QString;

class Q_MULTIMEDIA_EXPORT QVideoWindowControl : public QMediaControl
{
    Q_OBJECT

public:
    ~QVideoWindowControl();

    virtual WId winId() const = 0;
    virtual void setWinId(WId id) = 0;

    virtual QRect displayRect() const = 0;
    virtual void setDisplayRect(const QRect &rect) = 0;

    virtual bool isFullScreen() const = 0;
    virtual void setFullScreen(bool fullScreen) = 0;

    virtual void repaint() = 0;

    virtual QSize nativeSize() const = 0;

    virtual Qt::AspectRatioMode aspectRatioMode() const = 0;
    virtual void setAspectRatioMode(Qt::AspectRatioMode mode) = 0;

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
    void nativeSizeChanged();

protected:
    explicit QVideoWindowControl(QObject *parent = nullptr);
};

#define QVideoWindowControl_iid "org.qt-project.qt.videowindowcontrol/5.0"
Q_MEDIA_DECLARE_CONTROL(QVideoWindowControl, QVideoWindowControl_iid)

QT_END_NAMESPACE


#endif
