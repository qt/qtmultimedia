/****************************************************************************
**
** Copyright (C) 2016 Jolla Ltd, author: <robin.burchell@jollamobile.com>
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#include <QGlobalStatic>

#include <policy/resource.h>
#include <policy/resources.h>
#include <policy/resource-set.h>

#include "resourcepolicyimpl.h"
#include "resourcepolicyint.h"

Q_GLOBAL_STATIC(ResourcePolicyInt, globalResourcePolicyInt);

ResourcePolicyImpl::ResourcePolicyImpl(QObject *parent)
    : QMediaPlayerResourceSetInterface(parent)
{
    ResourcePolicyInt *set = globalResourcePolicyInt;
    set->addClient(this);
}

ResourcePolicyImpl::~ResourcePolicyImpl()
{
    ResourcePolicyInt *set = globalResourcePolicyInt;
    if (!globalResourcePolicyInt.isDestroyed())
        set->removeClient(this);
}

bool ResourcePolicyImpl::isVideoEnabled() const
{
    ResourcePolicyInt *set = globalResourcePolicyInt;
    return set->isVideoEnabled(this);
}

void ResourcePolicyImpl::setVideoEnabled(bool videoEnabled)
{
    ResourcePolicyInt *set = globalResourcePolicyInt;
    set->setVideoEnabled(this, videoEnabled);
}

void ResourcePolicyImpl::acquire()
{
    ResourcePolicyInt *set = globalResourcePolicyInt;
    set->acquire(this);
}

void ResourcePolicyImpl::release()
{
    ResourcePolicyInt *set = globalResourcePolicyInt;
    set->release(this);
}

bool ResourcePolicyImpl::isGranted() const
{
    ResourcePolicyInt *set = globalResourcePolicyInt;
    return set->isGranted(this);
}

bool ResourcePolicyImpl::isAvailable() const
{
    ResourcePolicyInt *set = globalResourcePolicyInt;
    return set->isAvailable();
}
