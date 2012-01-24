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

#include "qaudiosystemplugin.h"
#include "qaudiopluginloader_p.h"

#include <QtCore/qcoreapplication.h>
#include <QtCore/qpluginloader.h>
#include <QtCore/qfactoryinterface.h>
#include <QtCore/qdir.h>
#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

QAudioPluginLoader::QAudioPluginLoader(const char *iid, const QString &location, Qt::CaseSensitivity):
    m_iid(iid)
{
    m_location = location + QLatin1Char('/');
    load();
}

QAudioPluginLoader::~QAudioPluginLoader()
{
    for (int i = 0; i < m_plugins.count(); i++ ) {
        delete m_plugins.at(i);
    }
}

QStringList QAudioPluginLoader::pluginList() const
{
#if !defined QT_NO_DEBUG
    const bool showDebug = qgetenv("QT_DEBUG_PLUGINS").toInt() > 0;
#endif

    QStringList paths = QCoreApplication::libraryPaths();
#ifdef QTM_PLUGIN_PATH
    paths << QLatin1String(QTM_PLUGIN_PATH);
#endif
#if !defined QT_NO_DEBUG
    if (showDebug)
        qDebug() << "Plugin paths:" << paths;
#endif

    //temp variable to avoid multiple identic path
    QSet<QString> processed;

    /* Discover a bunch o plugins */
    QStringList plugins;

    /* Enumerate our plugin paths */
    for (int i=0; i < paths.count(); i++) {
        if (processed.contains(paths.at(i)))
            continue;
        processed.insert(paths.at(i));
        QDir pluginsDir(paths.at(i)+m_location);
        if (!pluginsDir.exists())
            continue;

        QStringList files = pluginsDir.entryList(QDir::Files);
#if !defined QT_NO_DEBUG
        if (showDebug)
            qDebug()<<"Looking for plugins in "<<pluginsDir.path()<<files;
#endif
        for (int j=0; j < files.count(); j++) {
            const QString &file = files.at(j);
            plugins <<  pluginsDir.absoluteFilePath(file);
        }
    }
    return  plugins;
}

QStringList QAudioPluginLoader::keys() const
{
    QMutexLocker locker(const_cast<QMutex *>(&m_mutex));

    QStringList list;
    for (int i = 0; i < m_plugins.count(); i++) {
        QAudioSystemPlugin* p = qobject_cast<QAudioSystemPlugin*>(m_plugins.at(i)->instance());
        if (p) list << p->keys();
    }

    return list;
}

QObject* QAudioPluginLoader::instance(QString const &key)
{
    QMutexLocker locker(&m_mutex);

    for (int i = 0; i < m_plugins.count(); i++) {
        QAudioSystemPlugin* p = qobject_cast<QAudioSystemPlugin*>(m_plugins.at(i)->instance());
        if (p && p->keys().contains(key))
            return m_plugins.at(i)->instance();
    }
    return 0;
}

QList<QObject*> QAudioPluginLoader::instances(QString const &key)
{
    QMutexLocker locker(&m_mutex);

    QList<QObject*> list;
    for (int i = 0; i < m_plugins.count(); i++) {
        QAudioSystemPlugin* p = qobject_cast<QAudioSystemPlugin*>(m_plugins.at(i)->instance());
        if (p && p->keys().contains(key))
            list << m_plugins.at(i)->instance();
    }
    return list;
}

void QAudioPluginLoader::load()
{
    if (!m_plugins.isEmpty())
        return;

#if !defined QT_NO_DEBUG
    const bool showDebug = qgetenv("QT_DEBUG_PLUGINS").toInt() > 0;
#endif

    QStringList plugins = pluginList();
    for (int i=0; i < plugins.count(); i++) {
        QPluginLoader* loader = new QPluginLoader(plugins.at(i));
        QObject *o = loader->instance();
        if (o != 0 && o->qt_metacast(m_iid) != 0) {
            m_plugins.append(loader);
        } else {
#if !defined QT_NO_DEBUG
            if (showDebug)
                qWarning() << "QAudioPluginLoader: Failed to load plugin: "
                           << plugins.at(i) << loader->errorString();
#endif
            delete o;
            //we are not calling loader->unload here for it may cause problem on some device
            delete loader;
        }
    }
}
QT_END_NAMESPACE

