/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
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
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qmediastreamscontrol.h"
#include "qmediacontrol_p.h"

QT_BEGIN_NAMESPACE

namespace
{
    class QMediaStreamsControlPrivateRegisterMetaTypes
    {
    public:
        QMediaStreamsControlPrivateRegisterMetaTypes()
        {
            qRegisterMetaType<QMediaStreamsControl::StreamType>();
        }
    } _registerMetaTypes;
}

/*!
    \class QMediaStreamsControl
    \inmodule QtMultimedia

    \ingroup multimedia
    \ingroup multimedia_control

    \brief The QMediaStreamsControl class provides a media stream selection control.


    The QMediaStreamsControl class provides descriptions of the available media streams
    and allows individual streams to be activated and deactivated.

    The interface name of QMediaStreamsControl is \c com.nokia.Qt.MediaStreamsControl as
    defined in QMediaStreamsControl_iid.

    \sa QMediaService::requestControl()
*/

/*!
    \macro QMediaStreamsControl_iid

    \c com.nokia.Qt.MediaStreamsControl

    Defines the interface name of the QMediaStreamsControl class.

    \relates QMediaStreamsControl
*/

/*!
    Constructs a new media streams control with the given \a parent.
*/
QMediaStreamsControl::QMediaStreamsControl(QObject *parent)
    :QMediaControl(*new QMediaControlPrivate, parent)
{
}

/*!
    Destroys a media streams control.
*/
QMediaStreamsControl::~QMediaStreamsControl()
{
}

/*!
  \enum QMediaStreamsControl::StreamType

  Media stream type.

  \value AudioStream Audio stream.
  \value VideoStream Video stream.
  \value SubPictureStream Subpicture or teletext stream.
  \value UnknownStream The stream type is unknown.
  \value DataStream
*/

/*!
    \fn QMediaStreamsControl::streamCount()

    Returns the number of media streams.
*/

/*!
    \fn QMediaStreamsControl::streamType(int stream)

    Return the type of a media \a stream.
*/

/*!
    \fn QMediaStreamsControl::metaData(int stream, const QString &key)

    Returns the meta-data value of \a key for a given \a stream.

    Useful metadata keys are QtMultimedia::MetaData::Title,
    QtMultimedia::MetaData::Description and QtMultimedia::MetaData::Language.
*/

/*!
    \fn QMediaStreamsControl::isActive(int stream)

    Returns true if the media \a stream is active.
*/

/*!
    \fn QMediaStreamsControl::setActive(int stream, bool state)

    Sets the active \a state of a media \a stream.

    Setting the active state of a media stream to true will activate it.  If any other stream
    of the same type was previously active it will be deactivated. Setting the active state fo a
    media stream to false will deactivate it.
*/

/*!
    \fn QMediaStreamsControl::streamsChanged()

    The signal is emitted when the available streams list is changed.
*/

/*!
    \fn QMediaStreamsControl::activeStreamsChanged()

    The signal is emitted when the active streams list is changed.
*/

#include "moc_qmediastreamscontrol.cpp"
QT_END_NAMESPACE

