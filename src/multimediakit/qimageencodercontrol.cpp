/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "qimageencodercontrol.h"
#include <QtCore/qstringlist.h>

QT_BEGIN_NAMESPACE

/*!
    \class QImageEncoderControl

    \inmodule QtMultimediaKit
    \ingroup multimedia-serv
    \since 1.0

    \brief The QImageEncoderControl class provides access to the settings of a media service that
    performs image encoding.

    If a QMediaService supports encoding image data it will implement QImageEncoderControl.
    This control allows to \l {setImageSettings()}{set image encoding settings} and
    provides functions for quering supported image \l {supportedImageCodecs()}{codecs} and
    \l {supportedResolutions()}{resolutions}.

    The interface name of QImageEncoderControl is \c com.nokia.Qt.QImageEncoderControl/1.0 as
    defined in QImageEncoderControl_iid.

    \sa QImageEncoderSettings, QMediaService::requestControl()
*/

/*!
    \macro QImageEncoderControl_iid

    \c com.nokia.Qt.QImageEncoderControl/1.0

    Defines the interface name of the QImageEncoderControl class.

    \relates QImageEncoderControl
*/

/*!
    Constructs a new image encoder control object with the given \a parent
*/
QImageEncoderControl::QImageEncoderControl(QObject *parent)
    :QMediaControl(parent)
{
}

/*!
    Destroys the image encoder control.
*/
QImageEncoderControl::~QImageEncoderControl()
{
}

/*!
    \fn QImageEncoderControl::supportedResolutions(const QImageEncoderSettings &settings = QImageEncoderSettings(),
                                                   bool *continuous = 0) const

    Returns a list of supported resolutions.

    If non null image \a settings parameter is passed,
    the returned list is reduced to resolutions supported with partial settings applied.
    It can be used to query the list of resolutions, supported by specific image codec.

    If the encoder supports arbitrary resolutions within the supported resolutions range,
    *\a continuous is set to true, otherwise *\a continuous is set to false.
    \since 1.0
*/

/*!
    \fn QImageEncoderControl::supportedImageCodecs() const

    Returns a list of supported image codecs.
    \since 1.0
*/

/*!
    \fn QImageEncoderControl::imageCodecDescription(const QString &codec) const

    Returns a description of an image \a codec.
    \since 1.0
*/

/*!
    \fn QImageEncoderControl::imageSettings() const

    Returns the currently used image encoder settings.

    The returned value may be different tha passed to QImageEncoderControl::setImageSettings()
    if the settings contains the default or undefined parameters.
    In this case if the undefined parameters are already resolved, they should be returned.
    \since 1.0
*/

/*!
    \fn QImageEncoderControl::setImageSettings(const QImageEncoderSettings &settings)

    Sets the selected image encoder \a settings.
    \since 1.0
*/

#include "moc_qimageencodercontrol.cpp"
QT_END_NAMESPACE

