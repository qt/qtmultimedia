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

#include "DebugMacros.h"

#include <qvideosurfaceformat.h>

#include "s60videosurface.h"
/*!
 * Constructs a video surface with the given \a parent.
*/

S60VideoSurface::S60VideoSurface(QObject *parent)
    : QAbstractVideoSurface(parent)
    , m_winId(0)
{
    DP0("S60VideoSurface::S60VideoSurface +++");
    DP0("S60VideoSurface::S60VideoSurface ---");
}

/*!
 * Destroys video surface.
*/

S60VideoSurface::~S60VideoSurface()
{
    DP0("S60VideoSurface::~S60VideoSurface +++");
    DP0("S60VideoSurface::~S60VideoSurface ---");
}

/*!
    \return the ID of the window a video surface end point renders to.
*/

WId S60VideoSurface::winId() const
{
    DP0("S60VideoSurface::winId");

    return m_winId;
}

/*!
   Sets the \a id of the window a video surface end point renders to.
*/

void S60VideoSurface::setWinId(WId id)
{
    DP0("S60VideoSurface::setWinId +++");

    m_winId = id;

    DP0("S60VideoSurface::setWinId ---");
}

/*!
    \return the sub-rect of a window where video is displayed.
*/

QRect S60VideoSurface::displayRect() const
{
    DP0("S60VideoSurface::displayRect");

    return m_displayRect;
}

/*!
    Sets the sub-\a rect of a window where video is displayed.
*/

void S60VideoSurface::setDisplayRect(const QRect &rect)
{
    DP0("S60VideoSurface::setDisplayRect +++");

    m_displayRect = rect;

    DP0("S60VideoSurface::setDisplayRect ---");
}

/*!
    \return the brightness adjustment applied to a video surface.

    Valid brightness values range between -100 and 100, the default is 0.
*/

int S60VideoSurface::brightness() const
{
    DP0("S60VideoSurface::brightness");

    return 0;
}

/*!
    Sets a \a brightness adjustment for a video surface.

    Valid brightness values range between -100 and 100, the default is 0.
*/

void S60VideoSurface::setBrightness(int brightness)
{
    DP0("S60VideoSurface::setBrightness +++");

    DP1("S60VideoSurface::setBrightness - brightness:", brightness);

    Q_UNUSED(brightness);

    DP0("S60VideoSurface::setBrightness ---");
}

/*!
   \return the contrast adjustment applied to a video surface.

    Valid contrast values range between -100 and 100, the default is 0.
*/

int S60VideoSurface::contrast() const
{
    DP0("S60VideoSurface::contrast");

    return 0;
}

/*!
    Sets the \a contrast adjustment for a video surface.

    Valid contrast values range between -100 and 100, the default is 0.
*/

void S60VideoSurface::setContrast(int contrast)
{
    DP0("S60VideoSurface::setContrast +++");

    DP1("S60VideoSurface::setContrast - ", contrast);

    Q_UNUSED(contrast);

    DP0("S60VideoSurface::setContrast ---");
}

/*!
   \return the hue adjustment applied to a video surface.

    Value hue values range between -100 and 100, the default is 0.
*/

int S60VideoSurface::hue() const
{
    DP0("S60VideoSurface::hue");

    return 0;
}

/*!
    Sets a \a hue adjustment for a video surface.

    Valid hue values range between -100 and 100, the default is 0.
*/

void S60VideoSurface::setHue(int hue)
{
    DP0("S60VideoSurface::setHue +++");

    DP1("S60VideoSurface::setHue - ", hue);

    Q_UNUSED(hue);

    DP0("S60VideoSurface::setHue ---");
}

/*!
    \return the saturation adjustment applied to a video surface.

    Value saturation values range between -100 and 100, the default is 0.
*/

int S60VideoSurface::saturation() const
{
    DP0("S60VideoSurface::saturation");

    return 0;
}

/*!
    Sets a \a saturation adjustment for a video surface.

    Valid saturation values range between -100 and 100, the default is 0.
*/

void S60VideoSurface::setSaturation(int saturation)
{
    DP0("S60VideoSurface::setSaturation +++");

    DP1("S60VideoSurface::setSaturation - ", saturation);

    Q_UNUSED(saturation);

    DP0("S60VideoSurface::setSaturation ---");
}

/*!
 * \return ZERO. \a attribute, \a minimum, \a maximum are not used.
*/
int S60VideoSurface::getAttribute(const char *attribute, int minimum, int maximum) const
{
    DP0("S60VideoSurface::getAttribute +++");

    Q_UNUSED(attribute);
    Q_UNUSED(minimum);
    Q_UNUSED(maximum);

    DP0("S60VideoSurface::getAttribute ---");

    return 0;
}

/*!
 * Sets the \a attribute, \a minimum, \a maximum.
 * But never used.
*/

void S60VideoSurface::setAttribute(const char *attribute, int value, int minimum, int maximum)
{
    DP0("S60VideoSurface::setAttribute +++");

    Q_UNUSED(attribute);
    Q_UNUSED(value);
    Q_UNUSED(minimum);
    Q_UNUSED(maximum);

    DP0("S60VideoSurface::setAttribute ---");

}

/*!
 * \return ZERO.
 * \a value, \a fromLower, \a fromUpper, \a toLower, \a toUpper are never used.
*/

int S60VideoSurface::redistribute(
        int value, int fromLower, int fromUpper, int toLower, int toUpper)
{
    DP0("S60VideoSurface::redistribute +++");

    Q_UNUSED(value);
    Q_UNUSED(fromLower);
    Q_UNUSED(fromUpper);
    Q_UNUSED(toLower);
    Q_UNUSED(toUpper);

    DP0("S60VideoSurface::redistribute ---");

    return 0;
}

/*!
 * \return List of video surface supported Pixel Formats.
*/

QList<QVideoFrame::PixelFormat> S60VideoSurface::supportedPixelFormats(
        QAbstractVideoBuffer::HandleType handleType) const
{
    DP0("S60VideoSurface::supportedPixelFormats +++");

    Q_UNUSED(handleType);
    QList<QVideoFrame::PixelFormat> list;

    DP0("S60VideoSurface::supportedPixelFormats ---");

    return list;
}

/*!
 * \return always FALSE, as \a format never used.
*/

bool S60VideoSurface::start(const QVideoSurfaceFormat &format)
{
    DP0("S60VideoSurface::start");

    Q_UNUSED(format);
    return false;
}

/*!
 * Stops video surface.
*/
void S60VideoSurface::stop()
{
    DP0("S60VideoSurface::stop +++");

    DP0("S60VideoSurface::stop ---");

}

/*!
 * \return always FALS, as \a format is never used.
*/
bool S60VideoSurface::present(const QVideoFrame &frame)
{
    DP0("S60VideoSurface::present");

    Q_UNUSED(frame);
    return false;
}

/*!
 * \return always FALSE.
*/

bool S60VideoSurface::findPort()
{
    DP0("S60VideoSurface::findPort");

    return false;
}

void S60VideoSurface::querySupportedFormats()
{
    DP0("S60VideoSurface::querySupportedFormats +++");

    DP0("S60VideoSurface::querySupportedFormats ---");

}

/*!
 * \return always FLASE, as \a format never used.
*/

bool S60VideoSurface::isFormatSupported(const QVideoSurfaceFormat &format) const
{
    DP0("S60VideoSurface::isFormatSupported");

    Q_UNUSED(format);
    return false;
}
