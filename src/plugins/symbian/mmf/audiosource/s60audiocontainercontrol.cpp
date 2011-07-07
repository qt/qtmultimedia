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

#include "s60audiocontainercontrol.h"
#include "s60audiocapturesession.h"
#include <QtCore/qdebug.h>

S60AudioContainerControl::S60AudioContainerControl(QObject *parent)
    : QMediaContainerControl(parent)
{
    DP0("S60AudioContainerControl::S60AudioContainerControl(QObject *parent) +++");

    DP0("S60AudioContainerControl::S60AudioContainerControl(QObject *parent) ---");

}

S60AudioContainerControl::S60AudioContainerControl(QObject *session, QObject *parent)
   : QMediaContainerControl(parent)
{
    DP0("S60AudioContainerControl::S60AudioContainerControl(QObject *session, QObject *parent) +++");

    m_session = qobject_cast<S60AudioCaptureSession*>(session);

    DP0("S60AudioContainerControl::S60AudioContainerControl(QObject *session, QObject *parent) ---");
}

QStringList S60AudioContainerControl::supportedContainers() const
{
    DP0("S60AudioContainerControl::supportedContainers");

    return m_session->supportedAudioContainers();
}

QString S60AudioContainerControl::containerMimeType() const
{
    DP0("S60AudioContainerControl::containerMimeType");

    return m_session->audioContainer();
}

void S60AudioContainerControl::setContainerMimeType(const QString &containerMimeType)
{
    DP0("S60AudioContainerControl::setContainerMimeType +++");

    m_session->setAudioContainer(containerMimeType);

    DP0("S60AudioContainerControl::setContainerMimeType ---");
}

QString S60AudioContainerControl::containerDescription(const QString &containerMimeType) const
{
    DP0("S60AudioContainerControl::containerDescription");

    return m_session->audioContainerDescription(containerMimeType);
}

