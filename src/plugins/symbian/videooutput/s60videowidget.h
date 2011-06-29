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

#ifndef S60VIDEOWIDGET_H
#define S60VIDEOWIDGET_H

#include <QtGui/QWidget>

QT_USE_NAMESPACE

class S60VideoWidget : public QWidget
{
    Q_OBJECT
public:
    S60VideoWidget(QWidget *parent = 0);
    ~S60VideoWidget();

    // QWidget
    bool event(QEvent *event);
    void paintEvent(QPaintEvent *event);
    void setVisible(bool visible);

    WId videoWinId() const;
    void setPixmap(const QPixmap *pixmap);
    void setWindowBackgroundColor();
    void setTopWinId(WId id);
    WId topWinId() const;
    void setOrdinalPosition(int ordinalPosition);
    int ordinalPosition() const;

public slots:
    void beginNativePaintEvent(const QRect &rect);
    void endNativePaintEvent(const QRect &rect);
    void setPaintingEnabled(bool enabled);
    void setFullScreen(bool enabled);
    void setContentRect(const QRect &rect);

signals:
    void beginVideoWidgetNativePaint();
    void endVideoWidgetNativePaint();

private:
    void updateOrdinalPosition();
    void queueReactivateWindow();

private slots:
    void reactivateWindow(QWidget *window);
    void setWindowsNonFading();

private:
    const QPixmap *m_pixmap;
    QRect m_contentRect;
    bool m_paintingEnabled;
    WId m_topWinId;
    int m_ordinalPosition;
};

#endif // S60VIDEOWIDGET_H

