/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QGSTREAMERVIDEOWIDGET_H
#define QGSTREAMERVIDEOWIDGET_H

#include <qvideowidgetcontrol.h>

#include "qgstreamervideorendererinterface.h"
#include <private/qgstreamerbushelper_p.h>

QT_BEGIN_NAMESPACE

class QGstreamerVideoWidget;

class QGstreamerVideoWidgetControl
        : public QVideoWidgetControl
        , public QGstreamerVideoRendererInterface
        , public QGstreamerSyncMessageFilter
        , public QGstreamerBusMessageFilter
{
    Q_OBJECT
    Q_INTERFACES(QGstreamerVideoRendererInterface QGstreamerSyncMessageFilter QGstreamerBusMessageFilter)
public:
    QGstreamerVideoWidgetControl(QObject *parent = 0);
    virtual ~QGstreamerVideoWidgetControl();

    GstElement *videoSink();

    QWidget *videoWidget();

    Qt::AspectRatioMode aspectRatioMode() const;
    void setAspectRatioMode(Qt::AspectRatioMode mode);

    bool isFullScreen() const;
    void setFullScreen(bool fullScreen);

    int brightness() const;
    void setBrightness(int brightness);

    int contrast() const;
    void setContrast(int contrast);

    int hue() const;
    void setHue(int hue);

    int saturation() const;
    void setSaturation(int saturation);

    void setOverlay();

    bool eventFilter(QObject *object, QEvent *event);
    bool processSyncMessage(const QGstreamerMessage &message);
    bool processBusMessage(const QGstreamerMessage &message);

public slots:
    void updateNativeVideoSize();

signals:
    void sinkChanged();
    void readyChanged(bool);

private:
    void createVideoWidget();
    void windowExposed();

    GstElement *m_videoSink;
    QGstreamerVideoWidget *m_widget;
    WId m_windowId;
    Qt::AspectRatioMode m_aspectRatioMode;
    bool m_fullScreen;
};

QT_END_NAMESPACE

#endif // QGSTREAMERVIDEOWIDGET_H
