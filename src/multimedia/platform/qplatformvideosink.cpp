/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
******************************************************************************/

#include "qplatformvideosink_p.h"

QT_BEGIN_NAMESPACE

/*!
    \class QPlatformVideoSink
    \internal

    \inmodule QtMultimedia

    \brief The QPlatformVideoSink class provides a media control for rendering video to a window.

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
    \fn QPlatformVideoSink::setWinId(WId id)

    Sets the \a id of the window a video overlay end point renders to.
*/

/*!
    \fn QPlatformVideoSink::setDisplayRect(const QRect &rect)
    Sets the sub-\a rect of a window where video is displayed.
*/

/*!
    \fn QPlatformVideoSink::setFullScreen(bool fullScreen)

    Sets whether a video overlay is a \a fullScreen overlay.
*/

/*!
    \fn QPlatformVideoSink::nativeSize() const

    Returns a suggested size for the video display based on the resolution and aspect ratio of the
    video.
*/

/*!
    \fn QPlatformVideoSink::setAspectRatioMode(Qt::AspectRatioMode mode)

    Sets the aspect ratio \a mode which determines how video is scaled to the fit the display region
    with respect to its aspect ratio.
*/

QT_END_NAMESPACE

#include "moc_qplatformvideosink_p.cpp"
