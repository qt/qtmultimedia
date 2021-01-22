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

#ifndef RESOURCEPOLICYIMPL_H
#define RESOURCEPOLICYIMPL_H

#include <QObject>

#include <private/qmediaresourceset_p.h>

namespace ResourcePolicy {
    class ResourceSet;
};

class ResourcePolicyImpl : public QMediaPlayerResourceSetInterface
{
    Q_OBJECT
public:
    ResourcePolicyImpl(QObject *parent = 0);
    ~ResourcePolicyImpl();

    bool isVideoEnabled() const;
    void setVideoEnabled(bool videoEnabled);
    void acquire();
    void release();
    bool isGranted() const;
    bool isAvailable() const;
};

#endif // RESOURCEPOLICYIMPL_H
