/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Mobility Components.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QGSTREAMERVIDEOWINDOW_H
#define QGSTREAMERVIDEOWINDOW_H

#include <qvideowindowcontrol.h>

#include "qgstreamervideorendererinterface.h"

QT_BEGIN_NAMESPACE
class QAbstractVideoSurface;
QT_END_NAMESPACE
class QX11VideoSurface;

#if defined(Q_WS_X11) && !defined(QT_NO_XVIDEO)

QT_USE_NAMESPACE

class QGstreamerVideoWindow : public QVideoWindowControl, public QGstreamerVideoRendererInterface
{
    Q_OBJECT
    Q_INTERFACES(QGstreamerVideoRendererInterface)
    Q_PROPERTY(QColor colorKey READ colorKey WRITE setColorKey)
    Q_PROPERTY(bool autopaintColorKey READ autopaintColorKey WRITE setAutopaintColorKey)
public:
    QGstreamerVideoWindow(QObject *parent = 0, const char *elementName = 0);
    ~QGstreamerVideoWindow();

    WId winId() const;
    void setWinId(WId id);

    QRect displayRect() const;
    void setDisplayRect(const QRect &rect);

    bool isFullScreen() const;
    void setFullScreen(bool fullScreen);

    QSize nativeSize() const;

    Qt::AspectRatioMode aspectRatioMode() const;
    void setAspectRatioMode(Qt::AspectRatioMode mode);

    QColor colorKey() const;
    void setColorKey(const QColor &);

    bool autopaintColorKey() const;
    void setAutopaintColorKey(bool);

    void repaint();

    int brightness() const;
    void setBrightness(int brightness);

    int contrast() const;
    void setContrast(int contrast);

    int hue() const;
    void setHue(int hue);

    int saturation() const;
    void setSaturation(int saturation);

    QAbstractVideoSurface *surface() const;

    GstElement *videoSink();

    void precessNewStream();
    bool isReady() const { return m_windowId != 0; }

signals:
    void sinkChanged();
    void readyChanged(bool);

private slots:
    void updateNativeVideoSize();

private:
    static void padBufferProbe(GstPad *pad, GstBuffer *buffer, gpointer user_data);

    GstElement *m_videoSink;
    WId m_windowId;
    Qt::AspectRatioMode m_aspectRatioMode;
    QRect m_displayRect;
    bool m_fullScreen;
    QSize m_nativeSize;
    mutable QColor m_colorKey;
    int m_bufferProbeId;
};

#endif //QT_NO_XVIDEO

#endif
