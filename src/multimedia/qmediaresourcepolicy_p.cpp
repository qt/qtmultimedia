/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qmediaresourcepolicy_p.h"
#include "qmediapluginloader_p.h"
#include "qmediaresourcepolicyplugin_p.h"
#include "qmediaresourceset_p.h"

namespace {
    class QDummyMediaPlayerResourceSet : public QMediaPlayerResourceSetInterface
    {
    public:
        QDummyMediaPlayerResourceSet(QObject *parent)
            : QMediaPlayerResourceSetInterface(parent)
        {
        }

        bool isVideoEnabled() const
        {
            return true;
        }

        bool isGranted() const
        {
            return true;
        }

        bool isAvailable() const
        {
            return true;
        }

        void acquire() {}
        void release() {}
        void setVideoEnabled(bool) {}
    };
}

QT_BEGIN_NAMESPACE

Q_GLOBAL_STATIC_WITH_ARGS(QMediaPluginLoader, resourcePolicyLoader,
        (QMediaResourceSetFactoryInterface_iid, QLatin1String("resourcepolicy"), Qt::CaseInsensitive))

Q_GLOBAL_STATIC(QObject, dummyRoot)

QObject* QMediaResourcePolicy::createResourceSet(const QString& interfaceId)
{
    QMediaResourceSetFactoryInterface *factory =
            qobject_cast<QMediaResourceSetFactoryInterface*>(resourcePolicyLoader()
                                                             ->instance(QLatin1String("default")));
    if (!factory)
        return 0;
    QObject* obj = factory->create(interfaceId);
    if (!obj) {
        if (interfaceId == QLatin1String(QMediaPlayerResourceSetInterface_iid)) {
            obj = new QDummyMediaPlayerResourceSet(dummyRoot());
        }
    }
    Q_ASSERT(obj);
    return obj;
}

void QMediaResourcePolicy::destroyResourceSet(QObject* resourceSet)
{
    if (resourceSet->parent() == dummyRoot()) {
        delete resourceSet;
        return;
    }
    QMediaResourceSetFactoryInterface *factory =
            qobject_cast<QMediaResourceSetFactoryInterface*>(resourcePolicyLoader()
                                                             ->instance(QLatin1String("default")));
    if (!factory)
        return;
    return factory->destroy(resourceSet);
}
QT_END_NAMESPACE
