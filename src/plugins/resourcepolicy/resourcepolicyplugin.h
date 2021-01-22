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

#ifndef RESOURCEPOLICYPLUGIN_H
#define RESOURCEPOLICYPLUGIN_H

#include <private/qmediaresourcepolicyplugin_p.h>
#include <QObject>

class ResourcePolicyPlugin : public QMediaResourcePolicyPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.qt.mediaresourcesetfactory/5.0" FILE "resourcepolicy.json")
    Q_INTERFACES(QMediaResourceSetFactoryInterface)
public:
    ResourcePolicyPlugin(QObject *parent = 0);

    QObject *create(const QString &interfaceId);
    void destroy(QObject *resourceSet);
};

#endif // RESOURCEPOLICYPLUGIN_H
