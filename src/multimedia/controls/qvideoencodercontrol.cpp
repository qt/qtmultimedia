/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qvideoencodercontrol.h"
#include <QtCore/qstringlist.h>
#include <QtCore/qvariant.h>

QT_BEGIN_NAMESPACE

/*!
    \class QVideoEncoderControl

    \inmodule QtMultimedia


    \ingroup multimedia_control

    \brief The QVideoEncoderControl class provides access to the settings
    of a media service that performs video encoding.

    If a QMediaService supports encoding video data it will implement
    QVideoEncoderControl.  This control provides information about the limits
    of restricted video encoder options and allows the selection of a set of
    video encoder settings as specified in a QVideoEncoderSettings object.

    The functionality provided by this control is exposed to application code
    through the QMediaRecorder class.

    The interface name of QVideoEncoderControl is \c org.qt-project.qt.videoencodercontrol/5.0 as
    defined in QVideoEncoderControl_iid.

    \sa QMediaRecorder, QVideoEncoderSettings, QMediaService::requestControl()
*/

/*!
    \macro QVideoEncoderControl_iid

    \c org.qt-project.qt.videoencodercontrol/5.0

    Defines the interface name of the QVideoEncoderControl class.

    \relates QVideoEncoderControl
*/

/*!
    Create a new video encoder control object with the given \a parent.
*/
QVideoEncoderControl::QVideoEncoderControl(QObject *parent)
    :QMediaControl(parent)
{
}

/*!
    Destroys a video encoder control.
*/
QVideoEncoderControl::~QVideoEncoderControl()
{
}

/*!
    \fn QVideoEncoderControl::supportedVideoCodecs() const

    Returns the list of supported video codecs.
*/

/*!
    \fn QVideoEncoderControl::videoCodecDescription(const QString &codec) const

    Returns a description of a video \a codec.
*/

/*!
    \fn QVideoEncoderControl::supportedResolutions(const QVideoEncoderSettings &settings = QVideoEncoderSettings(),
                                                   bool *continuous = 0) const

    Returns a list of supported resolutions.

    If non null video \a settings parameter is passed,
    the returned list is reduced to resolution supported with partial settings like
    \l {QVideoEncoderSettings::setCodec()}{video codec} or
    \l {QVideoEncoderSettings::setFrameRate()}{frame rate} applied.

    If the encoder supports arbitrary resolutions within the supported resolutions range,
    *\a continuous is set to true, otherwise *\a continuous is set to false.

    \sa QVideoEncoderSettings::resolution()
*/

/*!
    \fn QVideoEncoderControl::supportedFrameRates(const QVideoEncoderSettings &settings = QVideoEncoderSettings(),
                                                  bool *continuous = 0) const

    Returns a list of supported frame rates.

    If non null video \a settings parameter is passed,
    the returned list is reduced to frame rates supported with partial settings like
    \l {QVideoEncoderSettings::setCodec()}{video codec} or
    \l {QVideoEncoderSettings::setResolution()}{video resolution} applied.

    If the encoder supports arbitrary frame rates within the supported range,
    *\a continuous is set to true, otherwise *\a continuous is set to false.

    \sa QVideoEncoderSettings::frameRate()
*/

/*!
    \fn QVideoEncoderControl::videoSettings() const

    Returns the video encoder settings.

    The returned value may be different tha passed to QVideoEncoderControl::setVideoSettings()
    if the settings contains the default or undefined parameters.
    In this case if the undefined parameters are already resolved, they should be returned.
*/

/*!
    \fn QVideoEncoderControl::setVideoSettings(const QVideoEncoderSettings &settings)

    Sets the selected video encoder \a settings.
*/

#include "moc_qvideoencodercontrol.cpp"
QT_END_NAMESPACE

