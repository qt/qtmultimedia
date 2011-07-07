/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Mobility Components.
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

#include "DebugMacros.h"

#include "s60mediastreamcontrol.h"
#include "s60mediaplayersession.h"
#include "s60mediaplayercontrol.h"
#include <qmediastreamscontrol.h>

#include <QtCore/qdir.h>
#include <QtCore/qurl.h>
#include <QtCore/qdebug.h>

/*!
    Constructs a new media streams control with the given \a control.
*/

S60MediaStreamControl::S60MediaStreamControl(QObject *control, QObject *parent)
    : QMediaStreamsControl(parent)
    , m_control(NULL)
    , m_mediaType(S60MediaSettings::Unknown)
{
    DP0("S60MediaStreamControl::S60MediaStreamControl +++");

    m_control = qobject_cast<S60MediaPlayerControl*>(control);
    m_mediaType = m_control->mediaControlSettings().mediaType();

    DP0("S60MediaStreamControl::S60MediaStreamControl ---");
}

/*!
    Destroys a media streams control.
*/

S60MediaStreamControl::~S60MediaStreamControl()
{
    DP0("S60MediaStreamControl::~S60MediaStreamControl +++");
    DP0("S60MediaStreamControl::~S60MediaStreamControl ---");
}

/*!
    \return the number of media streams.
*/

int S60MediaStreamControl::streamCount()
{
    DP0("S60MediaStreamControl::streamCount");

    int streamCount = 0;
    if (m_control->isAudioAvailable())
        streamCount++;
    if (m_control->isVideoAvailable())
        streamCount++;
    DP1("S60MediaStreamControl::streamCount", streamCount);

    return streamCount;
}

/*!
    \return the type of a media \a streamNumber.
*/

QMediaStreamsControl::StreamType S60MediaStreamControl::streamType(int streamNumber)
{
    DP0("S60MediaStreamControl::streamType +++");

    DP1("S60MediaStreamControl::streamType - ", streamNumber);

    Q_UNUSED(streamNumber);

    QMediaStreamsControl::StreamType type = QMediaStreamsControl::UnknownStream;

    if (m_control->mediaControlSettings().mediaType() ==  S60MediaSettings::Video)
        type = QMediaStreamsControl::VideoStream;
    else
        type = QMediaStreamsControl::AudioStream;

    DP0("S60MediaStreamControl::streamType ---");

    return type;
}

/*!
    \return the meta-data value of \a key for a given \a streamNumber.

    Useful metadata keya are QtMultimediaKit::Title, QtMultimediaKit::Description and QtMultimediaKit::Language.
*/

QVariant S60MediaStreamControl::metaData(int streamNumber, QtMultimediaKit::MetaData key)
{
    DP0("S60MediaStreamControl::metaData");

    Q_UNUSED(streamNumber);

    if (m_control->session()) {
        if (m_control->session()->isMetadataAvailable())
            return m_control->session()->metaData(key);
    }
    return QVariant();
}

/*!
    \return true if the media \a streamNumber is active else false.
*/

bool S60MediaStreamControl::isActive(int streamNumber)
{
    DP0("S60MediaStreamControl::isActive +++");

    DP1("S60MediaStreamControl::isActive - ", streamNumber);

    if (m_control->mediaControlSettings().mediaType() ==  S60MediaSettings::Video) {
        switch (streamNumber) {
        case 1:
            return m_control->isVideoAvailable();
        case 2:
            return m_control->isAudioAvailable();
        default:
            break;
        }
    }

    DP0("S60MediaStreamControl::isActive ---");

    return m_control->isAudioAvailable();
}

/*!
    Sets the active \a streamNumber of a media \a state.

    Symbian MMF does not support enabling or disabling specific media streams.

    Setting the active state of a media stream to true will activate it.  If any other stream
    of the same type was previously active it will be deactivated. Setting the active state fo a
    media stream to false will deactivate it.
*/

void S60MediaStreamControl::setActive(int streamNumber, bool state)
{
    DP0("S60MediaStreamControl::setActive +++");

    DP2("S60MediaStreamControl::setActive - ", streamNumber, state);

    Q_UNUSED(streamNumber);
    Q_UNUSED(state);
    // Symbian MMF does not support enabling or disabling specific media streams

    DP0("S60MediaStreamControl::setActive ---");
}

/*!
     The signal is emitted when the available streams list is changed.
*/

void S60MediaStreamControl::handleStreamsChanged()
{
    DP0("S60MediaStreamControl::handleStreamsChanged +++");

    emit streamsChanged();

    DP0("S60MediaStreamControl::handleStreamsChanged ---");
}
