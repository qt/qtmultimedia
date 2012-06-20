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

#include "qaudioendpointselectorcontrol.h"

QT_BEGIN_NAMESPACE

/*!
    \class QAudioEndpointSelectorControl

    \brief The QAudioEndpointSelectorControl class provides an audio endpoint selector media control.
    \inmodule QtMultimedia
    \ingroup multimedia_control

    The QAudioEndpointSelectorControl class provides descriptions of the audio
    endpoints available on a system and allows one to be selected as the audio
    of a media service.

    The interface name of QAudioEndpointSelectorControl is \c org.qt-project.qt.audioendpointselectorcontrol/5.0 as
    defined in QAudioEndpointSelectorControl_iid.

    \sa QMediaService::requestControl()
*/

/*!
    \macro QAudioEndpointSelectorControl_iid

    \c org.qt-project.qt.audioendpointselectorcontrol/5.0

    Defines the interface name of the QAudioEndpointSelectorControl class.

    \relates QAudioEndpointSelectorControl
*/

/*!
    Constructs a new audio endpoint selector control with the given \a parent.
*/
QAudioEndpointSelectorControl::QAudioEndpointSelectorControl(QObject *parent)
    :QMediaControl(parent)
{
}

/*!
    Destroys an audio endpoint selector control.
*/
QAudioEndpointSelectorControl::~QAudioEndpointSelectorControl()
{
}

/*!
    \fn QList<QString> QAudioEndpointSelectorControl::availableEndpoints() const

    Returns a list of the names of the available audio endpoints.
*/

/*!
    \fn QString QAudioEndpointSelectorControl::endpointDescription(const QString& name) const

    Returns the description of the endpoint \a name.
*/

/*!
    \fn QString QAudioEndpointSelectorControl::defaultEndpoint() const

    Returns the name of the default audio endpoint.
*/

/*!
    \fn QString QAudioEndpointSelectorControl::activeEndpoint() const

    Returns the name of the currently selected audio endpoint.
*/

/*!
    \fn QAudioEndpointSelectorControl::setActiveEndpoint(const QString& name)

    Set the active audio endpoint to \a name.
*/

/*!
    \fn QAudioEndpointSelectorControl::activeEndpointChanged(const QString& name)

    Signals that the audio endpoint has changed to \a name.
*/

/*!
    \fn QAudioEndpointSelectorControl::availableEndpointsChanged()

    Signals that list of available endpoints has changed.
*/

#include "moc_qaudioendpointselectorcontrol.cpp"
QT_END_NAMESPACE

