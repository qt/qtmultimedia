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

#include <QDebug>

#include "resourcepolicyplugin.h"
#include "resourcepolicyimpl.h"

ResourcePolicyPlugin::ResourcePolicyPlugin(QObject *parent)
    : QMediaResourcePolicyPlugin(parent)
{
}

QObject *ResourcePolicyPlugin::create(const QString &interfaceId)
{
    // TODO: what is interfaceId for?
    return new ResourcePolicyImpl(this);
}

void ResourcePolicyPlugin::destroy(QObject *resourceSet)
{
    // TODO: do we need to do anything more elaborate here?
    delete resourceSet;
}

