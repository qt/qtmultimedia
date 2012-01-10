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

#include "camerabinresourcepolicy.h"

//#define DEBUG_RESOURCE_POLICY
#include <QtCore/qdebug.h>
#include <QtCore/qset.h>

#ifdef HAVE_RESOURCE_POLICY
#include <policy/resource.h>
#include <policy/resources.h>
#include <policy/resource-set.h>
#endif

CamerabinResourcePolicy::CamerabinResourcePolicy(QObject *parent) :
    QObject(parent),
    m_resourceSet(NoResources),
    m_releasingResources(false)
{
#ifdef HAVE_RESOURCE_POLICY
    //loaded resource set is also kept requested for image and video capture sets
    m_resource = new ResourcePolicy::ResourceSet("camera");
    m_resource->setAlwaysReply();
    m_resource->initAndConnect();

    connect(m_resource, SIGNAL(resourcesGranted(const QList<ResourcePolicy::ResourceType>)),
            SIGNAL(resourcesGranted()));
    connect(m_resource, SIGNAL(resourcesDenied()), SIGNAL(resourcesDenied()));
    connect(m_resource, SIGNAL(lostResources()), SIGNAL(resourcesLost()));
    connect(m_resource, SIGNAL(resourcesReleased()), SLOT(handleResourcesReleased()));
#endif
}

CamerabinResourcePolicy::~CamerabinResourcePolicy()
{
#ifdef HAVE_RESOURCE_POLICY
    //ensure the resources are released
    if (m_resourceSet != NoResources)
        setResourceSet(NoResources);

    //don't delete the resource set until resources are released
    if (m_releasingResources) {
        m_resource->connect(m_resource, SIGNAL(resourcesReleased()),
                            SLOT(deleteLater()));
    } else {
        delete m_resource;
        m_resource = 0;
    }
#endif
}

CamerabinResourcePolicy::ResourceSet CamerabinResourcePolicy::resourceSet() const
{
    return m_resourceSet;
}

void CamerabinResourcePolicy::setResourceSet(CamerabinResourcePolicy::ResourceSet set)
{
    CamerabinResourcePolicy::ResourceSet oldSet = m_resourceSet;
    m_resourceSet = set;

#ifdef DEBUG_RESOURCE_POLICY
    qDebug() << Q_FUNC_INFO << set;
#endif

#ifdef HAVE_RESOURCE_POLICY
    QSet<ResourcePolicy::ResourceType> requestedTypes;

    switch (set) {
    case NoResources:
        break;
    case LoadedResources:
        requestedTypes << ResourcePolicy::LensCoverType //to detect lens cover is opened/closed
                       << ResourcePolicy::VideoRecorderType //to open camera device
                       << ResourcePolicy::SnapButtonType; //to detect capture button events
        break;
    case ImageCaptureResources:
        requestedTypes << ResourcePolicy::LensCoverType
                       << ResourcePolicy::VideoPlaybackType
                       << ResourcePolicy::VideoRecorderType
                       << ResourcePolicy::AudioPlaybackType
                       << ResourcePolicy::ScaleButtonType
                       << ResourcePolicy::LedsType
                       << ResourcePolicy::SnapButtonType;
        break;
    case VideoCaptureResources:
        requestedTypes << ResourcePolicy::LensCoverType
                       << ResourcePolicy::VideoPlaybackType
                       << ResourcePolicy::VideoRecorderType
                       << ResourcePolicy::AudioPlaybackType
                       << ResourcePolicy::AudioRecorderType
                       << ResourcePolicy::ScaleButtonType
                       << ResourcePolicy::LedsType
                       << ResourcePolicy::SnapButtonType;
        break;
    }

    QSet<ResourcePolicy::ResourceType> currentTypes;
    foreach (ResourcePolicy::Resource *resource, m_resource->resources())
        currentTypes << resource->type();

    foreach (ResourcePolicy::ResourceType resourceType, currentTypes - requestedTypes)
        m_resource->deleteResource(resourceType);

    foreach (ResourcePolicy::ResourceType resourceType, requestedTypes - currentTypes) {
        if (resourceType == ResourcePolicy::LensCoverType) {
            ResourcePolicy::LensCoverResource *lensCoverResource = new ResourcePolicy::LensCoverResource;
            lensCoverResource->setOptional(true);
            m_resource->addResourceObject(lensCoverResource);
        } else {
            m_resource->addResource(resourceType);
        }
    }

    m_resource->update();
    if (set != NoResources) {
        m_resource->acquire();
    } else {
        if (oldSet != NoResources) {
            m_releasingResources = true;
            m_resource->release();
        }
    }
#endif
}

bool CamerabinResourcePolicy::isResourcesGranted() const
{
#ifdef HAVE_RESOURCE_POLICY
    foreach (ResourcePolicy::Resource *resource, m_resource->resources())
        if (!resource->isOptional() && !resource->isGranted())
            return false;
#endif
    return true;
}

void CamerabinResourcePolicy::handleResourcesReleased()
{
#ifdef HAVE_RESOURCE_POLICY
#ifdef DEBUG_RESOURCE_POLICY
    qDebug() << Q_FUNC_INFO;
#endif
    m_releasingResources = false;
#endif
}
