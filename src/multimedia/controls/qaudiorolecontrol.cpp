/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qmediacontrol_p.h"
#include "qaudiorolecontrol.h"

QT_BEGIN_NAMESPACE

/*!
    \class QAudioRoleControl
    \inmodule QtMultimedia
    \ingroup multimedia_control
    \since 5.6

    \brief The QAudioRoleControl class provides control over the audio role of a media object.

    If a QMediaService supports audio roles it will implement QAudioRoleControl.

    The functionality provided by this control is exposed to application code through the
    QMediaPlayer class.

    The interface name of QAudioRoleControl is \c org.qt-project.qt.audiorolecontrol/5.6 as
    defined in QAudioRoleControl_iid.

    \sa QMediaService::requestControl(), QMediaPlayer
*/

/*!
    \macro QAudioRoleControl_iid

    \c org.qt-project.qt.audiorolecontrol/5.6

    Defines the interface name of the QAudioRoleControl class.

    \relates QAudioRoleControl
*/

/*!
    Construct a QAudioRoleControl with the given \a parent.
*/
QAudioRoleControl::QAudioRoleControl(QObject *parent)
    : QMediaControl(*new QMediaControlPrivate, parent)
{

}

/*!
    Destroys the audio role control.
*/
QAudioRoleControl::~QAudioRoleControl()
{

}

/*!
    \fn QAudio::Role QAudioRoleControl::audioRole() const

    Returns the audio role of the media played by the media service.
*/

/*!
    \fn void QAudioRoleControl::setAudioRole(QAudio::Role role)

    Sets the audio \a role of the media played by the media service.
*/

/*!
    \fn QList<QAudio::Role> QAudioRoleControl::supportedAudioRoles() const

    Returns a list of audio roles that the media service supports.
*/

/*!
    \fn void QAudioRoleControl::audioRoleChanged(QAudio::Role role)

    Signal emitted when the audio \a role has changed.
 */


#include "moc_qaudiorolecontrol.cpp"
QT_END_NAMESPACE
