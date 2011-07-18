/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include <QtCore/qstring.h>
#include <QtCore/qfile.h>
#include <QtCore/qdebug.h>
#include <QtCore/qdir.h>

#include "v4lserviceplugin.h"
#include "v4lradioservice.h"

#include <qmediaserviceprovider.h>


QStringList V4LServicePlugin::keys() const
{
    return QStringList() <<
            QLatin1String(Q_MEDIASERVICE_RADIO);
}

QMediaService* V4LServicePlugin::create(QString const& key)
{
    if (key == QLatin1String(Q_MEDIASERVICE_RADIO))
        return new V4LRadioService;

    return 0;
}

void V4LServicePlugin::release(QMediaService *service)
{
    delete service;
}

QList<QByteArray> V4LServicePlugin::devices(const QByteArray &service) const
{
    return QList<QByteArray>();
}

QString V4LServicePlugin::deviceDescription(const QByteArray &service, const QByteArray &device)
{
    return QString();
}


Q_EXPORT_PLUGIN2(qtmedia_v4lengine, V4LServicePlugin);

