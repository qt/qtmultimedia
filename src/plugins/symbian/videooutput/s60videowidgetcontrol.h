/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef S60VIDEOWIDGETCONTROL_H
#define S60VIDEOWIDGETCONTROL_H

#include <qvideowidgetcontrol.h>

QT_USE_NAMESPACE

class S60VideoWidgetDisplay;

class S60VideoWidgetControl : public QVideoWidgetControl
{
    Q_OBJECT

    /**
     * WId of the topmost window in the application, used to calculate the
     * absolute ordinal position of the video widget.
     * This is used by the "window" implementation of QGraphicsVideoItem.
     */
    Q_PROPERTY(WId topWinId READ topWinId WRITE setTopWinId)

    /**
     * Ordinal position of the video widget, relative to the topmost window
     * in the application.  If both the topWinId property and the ordinalPosition
     * property are set, the absolute ordinal position of the video widget is
     * the sum of the topWinId ordinal position and the value of the
     * ordinalPosition property.
     * This is used by the "window" implementation of QGraphicsVideoItem.
     */
    Q_PROPERTY(int ordinalPosition READ ordinalPosition WRITE setOrdinalPosition)

    /**
     * Extent of the video, relative to this video widget.
     * This is used by the "window" implementation of QGraphicsVideoItem.
     */
    Q_PROPERTY(QRect extentRect READ extentRect WRITE setExtentRect)

    /**
     * Native size of video.
     * This is used by the "window" implementation of QGraphicsVideoItem.
     */
    Q_PROPERTY(QSize nativeSize READ nativeSize)

    /**
     * Rotation to be applied to video.
     * Angle is measured in degrees, with positive values counter-clockwise.
     * Zero is at 12 o'clock.
     */
    Q_PROPERTY(qreal rotation READ rotation WRITE setRotation)

public:
    S60VideoWidgetControl(QObject *parent);
    ~S60VideoWidgetControl();

public:
    // QVideoWidgetControl
    QWidget *videoWidget();
    Qt::AspectRatioMode aspectRatioMode() const;
    void setAspectRatioMode(Qt::AspectRatioMode ratio);
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

    S60VideoWidgetDisplay *display() const;

    WId topWinId() const;
    void setTopWinId(WId id);
    int ordinalPosition() const;
    void setOrdinalPosition(int ordinalPosition);
    const QRect &extentRect() const;
    void setExtentRect(const QRect &rect);
    QSize nativeSize() const;
    qreal rotation() const;
    void setRotation(qreal value);

signals:
    void nativeSizeChanged();

private:
    S60VideoWidgetDisplay *m_display;
};

#endif // S60VIDEOWIDGETCONTROL_H

