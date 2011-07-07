/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Mobility Components.
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

#ifndef S60VIDEOWINDOWCONTROL_H
#define S60VIDEOWINDOWCONTROL_H

#include <qvideowindowcontrol.h>

class S60VideoWindowDisplay;

QT_USE_NAMESPACE

class S60VideoWindowControl : public QVideoWindowControl
{
    Q_OBJECT

    /**
     * Rotation to be applied to video.
     * Angle is measured in degrees, with positive values counter-clockwise.
     * Zero is at 12 o'clock.
     */
    Q_PROPERTY(qreal rotation READ rotation WRITE setRotation)

public:
    S60VideoWindowControl(QObject *parent);
    ~S60VideoWindowControl();

public:
    // QVideoWindowControl
    WId winId() const;
    void setWinId(WId id);
    QRect displayRect() const;
    void setDisplayRect(const QRect &rect);
    bool isFullScreen() const;
    void setFullScreen(bool fullScreen);
    void repaint();
    QSize nativeSize() const;
    Qt::AspectRatioMode aspectRatioMode() const;
    void setAspectRatioMode(Qt::AspectRatioMode mode);
    QSize customAspectRatio() const;
    void setCustomAspectRatio(const QSize &customRatio);
    int brightness() const;
    void setBrightness(int brightness);
    int contrast() const;
    void setContrast(int contrast);
    int hue() const;
    void setHue(int hue);
    int saturation() const;
    void setSaturation(int saturation);

    S60VideoWindowDisplay *display() const;

    qreal rotation() const;
    void setRotation(qreal value);

public slots:
    void refreshDisplay();

private:
    S60VideoWindowDisplay *m_display;
};

#endif // S60VIDEOWINDOWCONTROL_H

