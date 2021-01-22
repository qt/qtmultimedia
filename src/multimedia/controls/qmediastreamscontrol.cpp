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

#include "qmediastreamscontrol.h"
#include "qmediacontrol_p.h"

QT_BEGIN_NAMESPACE

static void qRegisterMediaStreamControlMetaTypes()
{
    qRegisterMetaType<QMediaStreamsControl::StreamType>();
}

Q_CONSTRUCTOR_FUNCTION(qRegisterMediaStreamControlMetaTypes)


/*!
    \class QMediaStreamsControl
    \obsolete
    \inmodule QtMultimedia


    \ingroup multimedia_control

    \brief The QMediaStreamsControl class provides a media stream selection control.


    The QMediaStreamsControl class provides descriptions of the available media streams
    and allows individual streams to be activated and deactivated.

    The interface name of QMediaStreamsControl is \c org.qt-project.qt.mediastreamscontrol/5.0 as
    defined in QMediaStreamsControl_iid.

    \sa QMediaService::requestControl()
*/

/*!
    \macro QMediaStreamsControl_iid

    \c org.qt-project.qt.mediastreamscontrol/5.0

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
    \fn QMediaStreamsControl::streamType(int streamNumber)

    Return the type of media stream \a streamNumber.
*/

/*!
    \fn QMediaStreamsControl::metaData(int streamNumber, const QString &key)

    Returns the meta-data value of \a key for the given \a streamNumber.

    Useful metadata keys are QMediaMetaData::Title,
    QMediaMetaData::Description and QMediaMetaData::Language.
*/

/*!
    \fn QMediaStreamsControl::isActive(int streamNumber)

    Returns true if the media stream \a streamNumber is active.
*/

/*!
    \fn QMediaStreamsControl::setActive(int streamNumber, bool state)

    Sets the active \a state of media stream \a streamNumber.

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

QT_END_NAMESPACE

#include "moc_qmediastreamscontrol.cpp"
