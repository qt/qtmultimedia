/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qplatformvideosink_p.h"

QT_BEGIN_NAMESPACE

/*!
    \class QPlatformVideoSink
    \obsolete

    \inmodule QtMultimedia

    \ingroup multimedia_control
    \brief The QPlatformVideoSink class provides a media control for rendering video to a window.


    The winId() property QPlatformVideoSink allows a platform specific window
    ID to be set as the video render target.  The
    displayRect() property is used to set the region of the window the video
    should be rendered to, and the aspectRatioMode() property indicates how the
    video should be scaled to fit the displayRect().

    \snippet multimedia-snippets/video.cpp Video window control

    QPlatformVideoSink is one of a number of possible video output controls.

    \sa QVideoWidget
*/

/*!
    Constructs a new video window control with the given \a parent.
*/
QPlatformVideoSink::QPlatformVideoSink(QVideoSink *parent)
    : QObject(parent),
    sink(parent)
{
}

/*!
    \fn QPlatformVideoSink::winId() const

    Returns the ID of the window a video overlay end point renders to.
*/

/*!
    \fn QPlatformVideoSink::setWinId(WId id)

    Sets the \a id of the window a video overlay end point renders to.
*/

/*!
    \fn QPlatformVideoSink::displayRect() const
    Returns the sub-rect of a window where video is displayed.
*/

/*!
    \fn QPlatformVideoSink::setDisplayRect(const QRect &rect)
    Sets the sub-\a rect of a window where video is displayed.
*/

/*!
    \fn QPlatformVideoSink::isFullScreen() const

    Identifies if a video overlay is a fullScreen overlay.

    Returns true if the video overlay is fullScreen, and false otherwise.
*/

/*!
    \fn QPlatformVideoSink::setFullScreen(bool fullScreen)

    Sets whether a video overlay is a \a fullScreen overlay.
*/

/*!
    \fn QPlatformVideoSink::fullScreenChanged(bool fullScreen)

    Signals that the \a fullScreen state of a video overlay has changed.
*/

/*!
    \fn QPlatformVideoSink::repaint()

    Repaints the last frame.
*/

/*!
    \fn QPlatformVideoSink::nativeSize() const

    Returns a suggested size for the video display based on the resolution and aspect ratio of the
    video.
*/

/*!
    \fn QPlatformVideoSink::nativeSizeChanged()

    Signals that the native dimensions of the video have changed.
*/


/*!
    \fn QPlatformVideoSink::aspectRatioMode() const

    Returns how video is scaled to fit the display region with respect to its aspect ratio.
*/

/*!
    \fn QPlatformVideoSink::setAspectRatioMode(Qt::AspectRatioMode mode)

    Sets the aspect ratio \a mode which determines how video is scaled to the fit the display region
    with respect to its aspect ratio.
*/

/*!
    \fn QPlatformVideoSink::brightness() const

    Returns the brightness adjustment applied to a video overlay.

    Valid brightness values range between -100 and 100, the default is 0.
*/

/*!
    \fn QPlatformVideoSink::setBrightness(int brightness)

    Sets a \a brightness adjustment for a video overlay.

    Valid brightness values range between -100 and 100, the default is 0.
*/

/*!
    \fn QPlatformVideoSink::brightnessChanged(int brightness)

    Signals that a video overlay's \a brightness adjustment has changed.
*/

/*!
    \fn QPlatformVideoSink::contrast() const

    Returns the contrast adjustment applied to a video overlay.

    Valid contrast values range between -100 and 100, the default is 0.
*/

/*!
    \fn QPlatformVideoSink::setContrast(int contrast)

    Sets the \a contrast adjustment for a video overlay.

    Valid contrast values range between -100 and 100, the default is 0.
*/

/*!
    \fn QPlatformVideoSink::contrastChanged(int contrast)

    Signals that a video overlay's \a contrast adjustment has changed.
*/

/*!
    \fn QPlatformVideoSink::hue() const

    Returns the hue adjustment applied to a video overlay.

    Value hue values range between -100 and 100, the default is 0.
*/

/*!
    \fn QPlatformVideoSink::setHue(int hue)

    Sets a \a hue adjustment for a video overlay.

    Valid hue values range between -100 and 100, the default is 0.
*/

/*!
    \fn QPlatformVideoSink::hueChanged(int hue)

    Signals that a video overlay's \a hue adjustment has changed.
*/

/*!
    \fn QPlatformVideoSink::saturation() const

    Returns the saturation adjustment applied to a video overlay.

    Value saturation values range between -100 and 100, the default is 0.
*/

/*!
    \fn QPlatformVideoSink::setSaturation(int saturation)
    Sets a \a saturation adjustment for a video overlay.

    Valid saturation values range between -100 and 100, the default is 0.
*/

/*!
    \fn QPlatformVideoSink::saturationChanged(int saturation)

    Signals that a video overlay's \a saturation adjustment has changed.
*/

QT_END_NAMESPACE

#include "moc_qplatformvideosink_p.cpp"
