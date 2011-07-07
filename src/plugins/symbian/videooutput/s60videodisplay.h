/**
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

#ifndef S60VIDEODISPLAY_H
#define S60VIDEODISPLAY_H

#include <QtCore/QMetaType>
#include <QtCore/QObject>
#include <QtCore/QRect>
#include <QtCore/QSize>
#include <QtGui/qwindowdefs.h>

class CFbsBitmap;
class RWindow;

QT_USE_NAMESPACE

/*
 * This class defines a common API used by Symbian camera and mediaplayer
 * backends to render to on-screen video outputs, i.e. implementations of
 * QVideoWidgetControl and QVideoWindowControl.
 */
class S60VideoDisplay : public QObject
{
    Q_OBJECT
public:
    S60VideoDisplay(QObject *parent);
    virtual ~S60VideoDisplay();

    /*
     * Returns native Symbian handle of the window to be used for rendering
     */
    RWindow *windowHandle() const;

    /*
     * Returns Qt WId (CCoeControl* on Symbian)
     */
    virtual WId winId() const = 0;

    /*
     * Returns video display rectangle
     *
     * This is the rectangle which includes both the video content itself, plus
     * any border bars which added around the video.  The aspect ratio of this
     * rectangle therefore may differ from that of the nativeSize().
     *
     * If running on a platform supporting video rendering to graphics
     * surfaces (i.e. if VIDEOOUTPUT_GRAPHICS_SURFACES is defined), the return
     * value is the relative to the origin of the video window.  Otherwise, the
     * return value is an absolute screen rectangle.
     *
     * Note that this rectangle can extend beyond the bounds of the screen or of
     * the video window.
     *
     * When using QVideoWindowControl, the size of the extentRect matches the
     * displayRect; if running on a platform which supports only DSA rendering,
     * the origin differs as described above.
     *
     * See also clipRect, contentRect
     */
    virtual QRect extentRect() const = 0;

    /*
     * Returns video clipping rectangle
     *
     * This rectangle is the intersection of displayRect() with either the window
     * rectangle (on platforms supporting video rendering to graphics surfaces),
     * or the screen rectangle (on platforms supporting only DSA video rendering).
     *
     * If running on a platform supporting video rendering to graphics
     * surfaces (i.e. if VIDEOOUTPUT_GRAPHICS_SURFACES is defined), the return
     * value is the relative to the origin of the video window.  Otherwise, the
     * return value is an absolute screen rectangle.
     *
     * See also extentRect, contentRect
     */
    QRect clipRect() const;

    /*
     * Returns video content rectangle
     *
     * This is the rectangle in which the video content is rendered, i.e. its
     * size is that of extentRect() minus border bars.  The aspect ratio of this
     * rectangle is therefore equal to that of the nativeSize().
     *
     * This rectangle is always relative to the window in which video is rendered.
     *
     * See also extentRect, clipRect
     */
    QRect contentRect() const;

    void setFullScreen(bool enabled);
    bool isFullScreen() const;

    void setVisible(bool visible);
    bool isVisible() const;

    void setAspectRatioMode(Qt::AspectRatioMode mode);
    Qt::AspectRatioMode aspectRatioMode() const;

    const QSize& nativeSize() const;

    void setPaintingEnabled(bool enabled);
    bool isPaintingEnabled() const;

    void setRotation(qreal value);
    qreal rotation() const;

public slots:
    void setNativeSize(const QSize &size);

    /*
     * Provide new video frame
     *
     * If setPaintingEnabled(true) has been called, the frame is rendered to
     * the display.
     *
     * If a QWidget is available to the control (i.e. the control is a
     * QVideoWidgetControl), the frame is rendered via QPainter.  Otherwise, the
     * frame is blitted to the window using native Symbian drawing APIs.
     */
    virtual void setFrame(const CFbsBitmap &bitmap) = 0;

signals:
    void windowHandleChanged(RWindow *);
    void displayRectChanged(QRect extentRect, QRect clipRect);
    void fullScreenChanged(bool);
    void visibilityChanged(bool);
    void aspectRatioModeChanged(Qt::AspectRatioMode);
    void nativeSizeChanged(QSize);
    void contentRectChanged(QRect);
    void paintingEnabledChanged(bool);
    void rotationChanged(qreal);
    void beginVideoWindowNativePaint();
    void endVideoWindowNativePaint();

private slots:
    void updateContentRect();

private:
    QRect m_contentRect;
    bool m_fullScreen;
    bool m_visible;
    Qt::AspectRatioMode m_aspectRatioMode;
    QSize m_nativeSize;
    bool m_paintingEnabled;
    qreal m_rotation;
};

#endif // S60VIDEODISPLAY_H

