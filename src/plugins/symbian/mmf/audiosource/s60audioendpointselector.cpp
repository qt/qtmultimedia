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

#include "s60audiocapturesession.h"
#include "s60audioendpointselector.h"

#include <qaudiodeviceinfo.h>

S60AudioEndpointSelector::S60AudioEndpointSelector(QObject *session, QObject *parent)
    :QAudioEndpointSelector(parent)
{
    DP0("S60AudioEndpointSelector::S60AudioEndpointSelector +++");
    m_session = qobject_cast<S60AudioCaptureSession*>(session); 

    connect(m_session, SIGNAL(activeEndpointChanged(const QString &)), this, SIGNAL(activeEndpointChanged(const QString &)));

    DP0("S60AudioEndpointSelector::S60AudioEndpointSelector ---");
}

S60AudioEndpointSelector::~S60AudioEndpointSelector()
{
    DP0("S60AudioEndpointSelector::~S60AudioEndpointSelector +++");

    DP0("S60AudioEndpointSelector::~S60AudioEndpointSelector ---");
}

QList<QString> S60AudioEndpointSelector::availableEndpoints() const
{
    DP0("S60AudioEndpointSelector::availableEndpoints");

    return m_session->availableEndpoints();
}

QString S60AudioEndpointSelector::endpointDescription(const QString& name) const
{
    DP0("S60AudioEndpointSelector::endpointDescription");

    return m_session->endpointDescription(name);
}

QString S60AudioEndpointSelector::defaultEndpoint() const
{
    DP0("S60AudioEndpointSelector::defaultEndpoint");

    return m_session->defaultEndpoint();
}

QString S60AudioEndpointSelector::activeEndpoint() const
{
    DP0("S60AudioEndpointSelector::activeEndpoint");

    return m_session->activeEndpoint();
}

void S60AudioEndpointSelector::setActiveEndpoint(const QString& name)
{
   DP0("S60AudioEndpointSelector::setActiveEndpoint +++");
    m_session->setActiveEndpoint(name);
   DP0("S60AudioEndpointSelector::setActiveEndpoint ---");
}
