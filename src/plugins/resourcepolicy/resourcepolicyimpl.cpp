/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd, author: <robin.burchell@jollamobile.com>
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <policy/resource.h>
#include <policy/resources.h>
#include <policy/resource-set.h>

#include "resourcepolicyimpl.h"

ResourcePolicyImpl::ResourcePolicyImpl(QObject *parent)
    : QMediaPlayerResourceSetInterface(parent)
    , m_status(Initial)
    , m_videoEnabled(false)
{
    m_resourceSet = new ResourcePolicy::ResourceSet("player", this);
    m_resourceSet->setAlwaysReply();

    ResourcePolicy::AudioResource *audioResource = new ResourcePolicy::AudioResource("player");
    audioResource->setProcessID(QCoreApplication::applicationPid());
    audioResource->setStreamTag("media.name", "*");
    m_resourceSet->addResourceObject(audioResource);

    m_resourceSet->update();

    connect(m_resourceSet, SIGNAL(resourcesGranted(QList<ResourcePolicy::ResourceType>)),
            this, SLOT(handleResourcesGranted()));
    connect(m_resourceSet, SIGNAL(resourcesDenied()),
            this, SLOT(handleResourcesDenied()));
    connect(m_resourceSet, SIGNAL(lostResources()),
            this, SLOT(handleResourcesLost()));
    connect(m_resourceSet, SIGNAL(resourcesReleasedByManager()),
            this, SLOT(handleResourcesLost()));
}

bool ResourcePolicyImpl::isVideoEnabled() const
{
    return m_videoEnabled;
}

void ResourcePolicyImpl::setVideoEnabled(bool videoEnabled)
{
    if (m_videoEnabled != videoEnabled) {
        m_videoEnabled = videoEnabled;

        if (videoEnabled)
            m_resourceSet->addResource(ResourcePolicy::VideoPlaybackType);
        else
            m_resourceSet->deleteResource(ResourcePolicy::VideoPlaybackType);

        m_resourceSet->update();
    }
}

void ResourcePolicyImpl::acquire()
{
    m_status = RequestedResource;
    m_resourceSet->acquire();
}

void ResourcePolicyImpl::release()
{
    m_resourceSet->release();
    m_status = Initial;
}

bool ResourcePolicyImpl::isGranted() const
{
    return m_status == GrantedResource;
}

bool ResourcePolicyImpl::isAvailable() const
{
    // TODO: is this used? what is it for?
    qWarning() << Q_FUNC_INFO << "Stub";
    return true;
}

void ResourcePolicyImpl::handleResourcesGranted()
{
    m_status = GrantedResource;
    emit resourcesGranted();
}

void ResourcePolicyImpl::handleResourcesDenied()
{
    m_status = Initial;
    emit resourcesDenied();
}

void ResourcePolicyImpl::handleResourcesLost()
{
    if (m_status != Initial) {
        m_status = Initial;
        emit resourcesLost();
    }

    m_resourceSet->release();
}

