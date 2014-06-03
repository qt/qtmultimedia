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

#include "resourcepolicyint.h"
#include "resourcepolicyimpl.h"

#include <QMap>

static int clientid = 0;

ResourcePolicyInt::ResourcePolicyInt(QObject *parent)
    : QObject(parent)
    , m_acquired(0)
    , m_status(Initial)
    , m_video(0)
    , m_resourceSet(0)
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

ResourcePolicyInt::~ResourcePolicyInt()
{
    delete m_resourceSet;
    m_resourceSet = 0;
#ifdef RESOURCE_DEBUG
    qDebug() << "##### Tearing down singleton.";
#endif
}

void ResourcePolicyInt::addClient(ResourcePolicyImpl *client)
{
    clientEntry entry;
    entry.id = clientid++;
    entry.client = client;
    entry.status = Initial;
    entry.videoEnabled = false;
    m_clients.insert(entry.client, entry);
#ifdef RESOURCE_DEBUG
    qDebug() << "##### Add client " << client << " : " << entry.id;
#endif
}

void ResourcePolicyInt::removeClient(ResourcePolicyImpl *client)
{
    QMap<const ResourcePolicyImpl*, clientEntry>::iterator i = m_clients.find(client);
    if (i != m_clients.end()) {
#ifdef RESOURCE_DEBUG
        qDebug() << "##### Remove client " << client << " : " << i.value().id;
#endif
        if (i.value().status == GrantedResource)
            --m_acquired;
        m_clients.erase(i);
    }

    if (m_acquired == 0) {
#ifdef RESOURCE_DEBUG
        qDebug() << "##### Remove client, acquired = 0, release";
#endif
        m_resourceSet->release();
        m_status = Initial;
    }
}

bool ResourcePolicyInt::isVideoEnabled(const ResourcePolicyImpl *client) const
{
    QMap<const ResourcePolicyImpl*, clientEntry>::const_iterator i = m_clients.find(client);
    if (i != m_clients.constEnd())
        return i.value().videoEnabled;

    return false;
}

void ResourcePolicyInt::setVideoEnabled(const ResourcePolicyImpl *client, bool videoEnabled)
{
    bool update = false;

    QMap<const ResourcePolicyImpl*, clientEntry>::iterator i = m_clients.find(client);
    if (i != m_clients.end()) {
        if (videoEnabled == i.value().videoEnabled)
            return;

        if (videoEnabled) {
            if (m_video > 0) {
                i.value().videoEnabled = true;
            } else {
                m_resourceSet->addResource(ResourcePolicy::VideoPlaybackType);
                update = true;
            }
            ++m_video;
        } else {
            --m_video;
            i.value().videoEnabled = false;
            if (m_video == 0) {
                m_resourceSet->deleteResource(ResourcePolicy::VideoPlaybackType);
                update = true;
            }
        }
    }

    if (update)
        m_resourceSet->update();
}

void ResourcePolicyInt::acquire(const ResourcePolicyImpl *client)
{
    QMap<const ResourcePolicyImpl*, clientEntry>::iterator i = m_clients.find(client);
    if (i != m_clients.end()) {
#ifdef RESOURCE_DEBUG
        qDebug() << "##### " << i.value().id << ": ACQUIRE";
#endif
        if (i.value().status == Initial) {

            if (m_status == RequestedResource) {
                i.value().status = RequestedResource;
#ifdef RESOURCE_DEBUG
                qDebug() << "##### " << i.value().id << ": Already requesting, set client as RequestResource and return";
#endif
                return;
            }

            if (m_status == GrantedResource) {
                ++m_acquired;
#ifdef RESOURCE_DEBUG
                qDebug() << "##### " << i.value().id << ": Already granted, set as GrantedResource and return";
#endif
                i.value().status = GrantedResource;
                emit i.value().client->resourcesGranted();
                return;
            }
        } else if (i.value().status == RequestedResource) {
#ifdef RESOURCE_DEBUG
            qDebug() << "##### " << i.value().id << ": Already requesting, return";
#endif
            return;
        } else {
#ifdef RESOURCE_DEBUG
            qDebug() << "##### " << i.value().id << ": Already granted, return ";
#endif
            return;
        }
        i.value().status = RequestedResource;
        m_status = RequestedResource;

#ifdef RESOURCE_DEBUG
        qDebug() << "##### " << i.value().id << ": ACQUIRE call resourceSet->acquire()";
#endif
        // If m_status was Initial this is the first time resources are requested,
        // so let's actually do the acquiring
        m_resourceSet->acquire();
    }
}

void ResourcePolicyInt::release(const ResourcePolicyImpl *client)
{
    QMap<const ResourcePolicyImpl*, clientEntry>::iterator i = m_clients.find(client);
    if (i != m_clients.end()) {
        if (i.value().status == GrantedResource) {
            i.value().status = Initial;
            --m_acquired;
#ifdef RESOURCE_DEBUG
            qDebug() << "##### " << i.value().id << ": RELEASE, acquired (" << m_acquired << ")";
#endif
        }
    }

    if (m_acquired == 0) {
#ifdef RESOURCE_DEBUG
        qDebug() << "##### " << i.value().id << ": RELEASE call resourceSet->release()";
#endif
        m_resourceSet->release();
        m_status = Initial;
    }
}

bool ResourcePolicyInt::isGranted(const ResourcePolicyImpl *client) const
{
    QMap<const ResourcePolicyImpl*, clientEntry>::const_iterator i = m_clients.find(client);
    if (i != m_clients.constEnd()) {
#ifdef RESOURCE_DEBUG
            qDebug() << "##### " << i.value().id << ": IS GRANTED, status: " << i.value().status;
#endif
        return i.value().status == GrantedResource;
    }

    return false;
}

bool ResourcePolicyInt::isAvailable() const
{
    // TODO: is this used? what is it for?
    qWarning() << Q_FUNC_INFO << "Stub";
    return true;
}

void ResourcePolicyInt::handleResourcesGranted()
{
    m_status = GrantedResource;
    QMap<const ResourcePolicyImpl*, clientEntry>::iterator i = m_clients.begin();
    while (i != m_clients.end()) {
        if (i.value().status == RequestedResource) {
            ++m_acquired;
#ifdef RESOURCE_DEBUG
            qDebug() << "##### " << i.value().id << ": HANDLE GRANTED, acquired (" << m_acquired << ") emitting resourcesGranted()";
#endif
            i.value().status = GrantedResource;
            emit i.value().client->resourcesGranted();
        }
        ++i;
    }
}

void ResourcePolicyInt::handleResourcesDenied()
{
    m_status = Initial;
    m_acquired = 0;
    QMap<const ResourcePolicyImpl*, clientEntry>::iterator i = m_clients.begin();
    while (i != m_clients.end()) {
        if (i.value().status == RequestedResource) {
#ifdef RESOURCE_DEBUG
            qDebug() << "##### " << i.value().id << ": HANDLE DENIED, acquired (" << m_acquired << ") emitting resourcesDenied()";
#endif
            i.value().status = Initial;
            emit i.value().client->resourcesDenied();
        }
        // Do we need to act for clients that are in granted state?
        ++i;
    }
}

void ResourcePolicyInt::handleResourcesLost()
{
    if (m_status != Initial) {
        m_status = Initial;
    }

    m_acquired = 0;
    m_resourceSet->release();

    QMap<const ResourcePolicyImpl*, clientEntry>::iterator i = m_clients.begin();
    while (i != m_clients.end()) {
        if (i.value().status != Initial) {
#ifdef RESOURCE_DEBUG
            qDebug() << "##### " << i.value().id << ": HANDLE LOST, acquired (" << m_acquired << ") emitting resourcesLost()";
#endif
            i.value().status = Initial;
            emit i.value().client->resourcesLost();
        }
        ++i;
    }
}
