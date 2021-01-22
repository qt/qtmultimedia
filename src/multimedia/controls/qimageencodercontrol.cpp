/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
****************************************************************************/

#include "qimageencodercontrol.h"
#include <QtCore/qstringlist.h>

QT_BEGIN_NAMESPACE

/*!
    \class QImageEncoderControl
    \obsolete

    \inmodule QtMultimedia


    \ingroup multimedia_control

    \brief The QImageEncoderControl class provides access to the settings of a media service that
    performs image encoding.

    If a QMediaService supports encoding image data it will implement QImageEncoderControl.
    This control allows to \l {setImageSettings()}{set image encoding settings} and
    provides functions for quering supported image \l {supportedImageCodecs()}{codecs} and
    \l {supportedResolutions()}{resolutions}.

    The interface name of QImageEncoderControl is \c org.qt-project.qt.imageencodercontrol/5.0 as
    defined in QImageEncoderControl_iid.

    \sa QImageEncoderSettings, QMediaService::requestControl()
*/

/*!
    \macro QImageEncoderControl_iid

    \c org.qt-project.qt.imageencodercontrol/5.0

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
*/

/*!
    \fn QImageEncoderControl::supportedImageCodecs() const

    Returns a list of supported image codecs.
*/

/*!
    \fn QImageEncoderControl::imageCodecDescription(const QString &codec) const

    Returns a description of an image \a codec.
*/

/*!
    \fn QImageEncoderControl::imageSettings() const

    Returns the currently used image encoder settings.

    The returned value may be different tha passed to QImageEncoderControl::setImageSettings()
    if the settings contains the default or undefined parameters.
    In this case if the undefined parameters are already resolved, they should be returned.
*/

/*!
    \fn QImageEncoderControl::setImageSettings(const QImageEncoderSettings &settings)

    Sets the selected image encoder \a settings.
*/

QT_END_NAMESPACE

#include "moc_qimageencodercontrol.cpp"
