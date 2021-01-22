/****************************************************************************
**
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

#ifndef CAMERARESOURCEPOLICY_H
#define CAMERARESOURCEPOLICY_H

#include <QtCore/qobject.h>

namespace ResourcePolicy {
class ResourceSet;
};

QT_BEGIN_NAMESPACE

class CamerabinResourcePolicy : public QObject
{
    Q_OBJECT
public:
    enum ResourceSet {
        NoResources,
        LoadedResources,
        ImageCaptureResources,
        VideoCaptureResources
    };

    CamerabinResourcePolicy(QObject *parent);
    ~CamerabinResourcePolicy();

    ResourceSet resourceSet() const;
    void setResourceSet(ResourceSet set);

    bool isResourcesGranted() const;

    bool canCapture() const;

Q_SIGNALS:
    void resourcesDenied();
    void resourcesGranted();
    void resourcesLost();
    void canCaptureChanged();

private Q_SLOTS:
    void handleResourcesLost();
    void handleResourcesGranted();
    void handleResourcesReleased();
    void resourcesAvailable();
    void updateCanCapture();


private:
    ResourceSet m_resourceSet;
    ResourcePolicy::ResourceSet *m_resource;
    bool m_releasingResources;
    bool m_canCapture;
};

QT_END_NAMESPACE

#endif
