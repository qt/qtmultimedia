/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qmediapluginloader_p.h"
#include <QtCore/qcoreapplication.h>
#include <QtCore/qdebug.h>
#include <QtCore/qjsonarray.h>
#include <private/qfactoryloader_p.h>

#include "qmediaserviceproviderplugin.h"

QT_BEGIN_NAMESPACE

QMediaPluginLoader::QMediaPluginLoader(const char *iid, const QString &location, Qt::CaseSensitivity caseSensitivity):
    m_iid(iid)
{
#if defined(Q_OS_ANDROID)
    m_location = QString(location).replace(QLatin1Char('/'), QLatin1Char('_'));
#else
    m_location = QString::fromLatin1("/%1").arg(location);
#endif
    m_factoryLoader = new QFactoryLoader(m_iid, m_location, caseSensitivity);
    loadMetadata();
}

QMediaPluginLoader::~QMediaPluginLoader()
{
    delete m_factoryLoader;
}

QStringList QMediaPluginLoader::keys() const
{
    return m_map.keys();
}

QObject *QMediaPluginLoader::instance(QString const &key)
{
    if (!m_map.contains(key))
        return nullptr;

    int idx = m_map.value(key);
    if (idx < 0)
        return nullptr;

    return m_factoryLoader->instance(idx);
}

void QMediaPluginLoader::loadMetadata()
{
#if !defined QT_NO_DEBUG
    const bool showDebug = qgetenv("QT_DEBUG_PLUGINS").toInt() > 0;
#endif

#if !defined QT_NO_DEBUG
        if (showDebug)
            qDebug() << "QMediaPluginLoader: loading metadata for iid " << m_iid << " at location " << m_location;
#endif

    if (!m_map.isEmpty()) {
#if !defined QT_NO_DEBUG
        if (showDebug)
            qDebug() << "QMediaPluginLoader: already loaded metadata, returning";
#endif
        return;
    }

    QList<QJsonObject> meta = m_factoryLoader->metaData();
    for (int i = 0; i < meta.size(); i++) {
        QJsonObject jsonobj = meta.at(i).value(QStringLiteral("MetaData")).toObject();
        QJsonArray arr = jsonobj.value(QStringLiteral("Services")).toArray();

        for (const QJsonValue &value : qAsConst(arr)) {
            QString key = value.toString();

            if (m_map.contains(key)) {
#if !defined QT_NO_DEBUG
                if (showDebug)
                    qDebug() << "QMediaPluginLoader: Two plugins provide service " << key;
#endif
                continue;
            }

#if !defined QT_NO_DEBUG
            if (showDebug)
                qDebug() << "QMediaPluginLoader: Inserting new list for key: " << key;
#endif
            m_map.insert(key, i);
        }
    }
}

QT_END_NAMESPACE

