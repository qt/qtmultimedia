/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Mobility Components.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "DebugMacros.h"

#include "s60mediaplayercontrol.h"
#include "s60mediaplayersession.h"
#include "s60mediaplayeraudioendpointselector.h"

#include <QtGui/QIcon>
#include <QtCore/QDebug>

/*!
    Constructs a new audio endpoint selector with the given \a parent.
*/

S60MediaPlayerAudioEndpointSelector::S60MediaPlayerAudioEndpointSelector(QObject *control, QObject *parent)
   :QAudioEndpointSelector(parent)
    , m_control(0)
{
    DP0("S60MediaPlayerAudioEndpointSelector::S60MediaPlayerAudioEndpointSelector +++");

    m_control = qobject_cast<S60MediaPlayerControl*>(control);
    m_audioEndpointNames.append("Default");
    m_audioEndpointNames.append("All");
    m_audioEndpointNames.append("None");
    m_audioEndpointNames.append("Earphone");
    m_audioEndpointNames.append("Speaker");

    DP0("S60MediaPlayerAudioEndpointSelector::S60MediaPlayerAudioEndpointSelector ---");
}

/*!
    Destroys an audio endpoint selector.
*/

S60MediaPlayerAudioEndpointSelector::~S60MediaPlayerAudioEndpointSelector()
{
    DP0("S60MediaPlayerAudioEndpointSelector::~S60MediaPlayerAudioEndpointSelector +++");
    DP0("S60MediaPlayerAudioEndpointSelector::~S60MediaPlayerAudioEndpointSelector ---");
}

/*!
    \return a list of available audio endpoints.
*/

QList<QString> S60MediaPlayerAudioEndpointSelector::availableEndpoints() const
{
    DP0("S60MediaPlayerAudioEndpointSelector::availableEndpoints");

    return m_audioEndpointNames;
}

/*!
    \return the description of the endpoint name.
*/

QString S60MediaPlayerAudioEndpointSelector::endpointDescription(const QString& name) const
{
    DP0("S60MediaPlayerAudioEndpointSelector::endpointDescription");

    if (name == QString("Default")) //ENoPreference
        return QString("Used to indicate that the playing audio can be routed to"
            "any speaker. This is the default value for audio.");
    else if (name == QString("All")) //EAll
        return QString("Used to indicate that the playing audio should be routed to all speakers.");
    else if (name == QString("None")) //ENoOutput
        return QString("Used to indicate that the playing audio should not be routed to any output.");
    else if (name == QString("Earphone")) //EPrivate
        return QString("Used to indicate that the playing audio should be routed to"
            "the default private speaker. A private speaker is one that can only"
            "be heard by one person.");
    else if (name == QString("Speaker")) //EPublic
        return QString("Used to indicate that the playing audio should be routed to"
            "the default public speaker. A public speaker is one that can "
            "be heard by multiple people.");

    return QString();
}

/*!
    \return the name of the currently selected audio endpoint.
*/

QString S60MediaPlayerAudioEndpointSelector::activeEndpoint() const
{
    DP0("S60MediaPlayerAudioEndpointSelector::activeEndpoint");

    if (m_control->session()) {
        DP1("S60MediaPlayerAudioEndpointSelector::activeEndpoint - ",
                m_control->session()->activeEndpoint());
        return m_control->session()->activeEndpoint();
    }
    else {
        DP1("S60MediaPlayerAudioEndpointSelector::activeEndpoint - ",
                m_control->mediaControlSettings().audioEndpoint());
        return m_control->mediaControlSettings().audioEndpoint();
    }
}

/*!
    \return the name of the default audio endpoint.
*/

QString S60MediaPlayerAudioEndpointSelector::defaultEndpoint() const
{
    DP0("S60MediaPlayerAudioEndpointSelector::defaultEndpoint");

    if (m_control->session()) {
        DP1("S60MediaPlayerAudioEndpointSelector::defaultEndpoint - ",
                m_control->session()->defaultEndpoint());
        return m_control->session()->defaultEndpoint();
    }
    else {
        DP1("S60MediaPlayerAudioEndpointSelector::defaultEndpoint - ",
                m_control->mediaControlSettings().audioEndpoint());
        return m_control->mediaControlSettings().audioEndpoint();
    }
}

/*!
    Set the audio endpoint to \a name.
*/

void S60MediaPlayerAudioEndpointSelector::setActiveEndpoint(const QString& name)
{
    DP0("S60MediaPlayerAudioEndpointSelector::setActiveEndpoin +++");

    DP1("S60MediaPlayerAudioEndpointSelector::setActiveEndpoint - ", name);

    QString oldEndpoint = m_control->mediaControlSettings().audioEndpoint();

    if (name != oldEndpoint && (name == QString("Default") || name == QString("All") ||
        name == QString("None") || name == QString("Earphone") || name == QString("Speaker"))) {

        if (m_control->session()) {
            m_control->session()->setActiveEndpoint(name);
            emit activeEndpointChanged(name);
        }
        m_control->setAudioEndpoint(name);
    }

    DP0("S60MediaPlayerAudioEndpointSelector::setActiveEndpoin ---");
}
