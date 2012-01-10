/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#include "qmediarecordercontrol.h"

QT_BEGIN_NAMESPACE


/*!
    \class QMediaRecorderControl
    \inmodule QtMultimedia
    \ingroup multimedia-serv
    \since 1.0


    \brief The QMediaRecorderControl class provides access to the recording
    functionality of a QMediaService.

    If a QMediaService can record media it will implement QMediaRecorderControl.
    This control provides a means to set the \l {outputLocation()}{output location},
    and \l {record()}{start}, \l {pause()}{pause} and \l {stop()}{stop}
    recording.  It also provides feedback on the \l {duration()}{duration}
    of the recording.

    The functionality provided by this control is exposed to application
    code through the QMediaRecorder class.

    The interface name of QMediaRecorderControl is \c com.nokia.Qt.QMediaRecorderControl/1.0 as
    defined in QMediaRecorderControl_iid.

    \sa QMediaService::requestControl(), QMediaRecorder

*/

/*!
    \macro QMediaRecorderControl_iid

    \c com.nokia.Qt.QMediaRecorderControl/1.0

    Defines the interface name of the QMediaRecorderControl class.

    \relates QMediaRecorderControl
*/

/*!
    Constructs a media recorder control with the given \a parent.
*/

QMediaRecorderControl::QMediaRecorderControl(QObject* parent)
    : QMediaControl(parent)
{
}

/*!
    Destroys a media recorder control.
*/

QMediaRecorderControl::~QMediaRecorderControl()
{
}

/*!
    \fn QUrl QMediaRecorderControl::outputLocation() const

    Returns the current output location being used.
    \since 1.0
*/

/*!
    \fn bool QMediaRecorderControl::setOutputLocation(const QUrl &location)

    Sets the output \a location and returns if this operation is successful.
    If file at the output location already exists, it should be overwritten.

    The \a location can be relative or empty;
    in this case the service should use the system specific place and file naming scheme.
    After recording has stated, QMediaRecorderControl::outputLocation() should return the actual output location.
    \since 1.0
*/

/*!
    \fn int QMediaRecorderControl::state() const

    Return the current recording state.
    \since 1.0
*/

/*!
    \fn qint64 QMediaRecorderControl::duration() const

    Return the current duration in milliseconds.
    \since 1.0
*/

/*!
    \fn void QMediaRecorderControl::record()

    Start recording.
    \since 1.0
*/

/*!
    \fn void QMediaRecorderControl::pause()

    Pause recording.
    \since 1.0
*/

/*!
    \fn void QMediaRecorderControl::stop()

    Stop recording.
    \since 1.0
*/

/*!
    \fn void QMediaRecorderControl::applySettings()

    Commits the encoder settings and performs pre-initialization to reduce delays when recording
    is started.
    \since 1.0
*/

/*!
    \fn bool QMediaRecorderControl::isMuted() const

    Returns true if the recorder is muted, and false if it is not.
    \since 1.0
*/

/*!
    \fn void QMediaRecorderControl::setMuted(bool muted)

    Sets the \a muted state of a media recorder.
    \since 1.0
*/


/*!
    \fn void QMediaRecorderControl::stateChanged(QMediaRecorder::State state)

    Signals that the \a state of a media recorder has changed.
    \since 1.0
*/

/*!
    \fn void QMediaRecorderControl::durationChanged(qint64 duration)

    Signals that the \a duration of the recorded media has changed.

    This only emitted when there is a discontinuous change in the duration such as being reset to 0.
    \since 1.0
*/

/*!
    \fn void QMediaRecorderControl::mutedChanged(bool muted)

    Signals that the \a muted state of a media recorder has changed.
    \since 1.0
*/

/*!
    \fn void QMediaRecorderControl::error(int error, const QString &errorString)

    Signals that an \a error has occurred.  The \a errorString describes the error.
    \since 1.0
*/

#include "moc_qmediarecordercontrol.cpp"
QT_END_NAMESPACE

