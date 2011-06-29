/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Mobility Components.
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

#include "qmediapluginloader_p.h"
#include <QtCore/qcoreapplication.h>
#include <QtCore/qpluginloader.h>
#include <QtCore/qdir.h>
#include <QtCore/qdebug.h>

#include "qmediaserviceproviderplugin.h"

#if defined(Q_OS_SYMBIAN)
# include <f32file.h>
#endif

#if defined(Q_OS_MAC)
# include <CoreFoundation/CoreFoundation.h>
#endif

QT_BEGIN_NAMESPACE

typedef QMap<QString,QObjectList> ObjectListMap;
Q_GLOBAL_STATIC(ObjectListMap, staticMediaPlugins);


#if defined(Q_OS_SYMBIAN)
// XXX: Copied over from Mobility, hopefully to be removed at some point
class DirChecker
{
public:
    DirChecker();
    ~DirChecker();
    bool checkDir(const QDir& dir);

private:
    RFs rfs;
};

DirChecker::DirChecker()
{
    qt_symbian_throwIfError(rfs.Connect());
}

bool DirChecker::checkDir(const QDir& dir)
{
    bool pathFound = false;
    // In Symbian, going cdUp() in a c:/private/<uid3>/ will result in *platsec* error at fileserver (requires AllFiles capability)
    // Also, trying to cd() to a nonexistent directory causes *platsec* error. This does not cause functional harm, but should
    // nevertheless be changed to use native Symbian methods to avoid unnecessary platsec warnings (as per qpluginloader.cpp).
    // Use native Symbian code to check for directory existence, because checking
    // for files from under non-existent protected dir like E:/private/<uid> using
    // QDir::exists causes platform security violations on most apps.
    QString nativePath = QDir::toNativeSeparators(dir.absolutePath());
    TPtrC ptr = TPtrC16(static_cast<const TUint16*>(nativePath.utf16()), nativePath.length());
    TUint attributes;
    TInt err = rfs.Att(ptr, attributes);
    if (err == KErrNone) {
        // yes, the directory exists.
        pathFound = true;
    }
    return pathFound;
}

DirChecker::~DirChecker()
{
    rfs.Close();
}
#endif


QMediaPluginLoader::QMediaPluginLoader(const char *iid, const QString &location, Qt::CaseSensitivity):
    m_iid(iid)
{
    m_location = QString::fromLatin1("/%1").arg(location);
    load();
}

QStringList QMediaPluginLoader::keys() const
{
    return m_instances.keys();
}

QObject* QMediaPluginLoader::instance(QString const &key)
{
    return m_instances.value(key).value(0);
}

QList<QObject*> QMediaPluginLoader::instances(QString const &key)
{
    return m_instances.value(key);
}

//to be used for testing purposes only
void QMediaPluginLoader::setStaticPlugins(const QString &location, const QObjectList& objects)
{
    staticMediaPlugins()->insert(QString::fromLatin1("/%1").arg(location), objects);
}

QStringList QMediaPluginLoader::availablePlugins() const
{
    QStringList paths;
    QStringList plugins;

#if defined(Q_OS_SYMBIAN)
    DirChecker dirChecker;
#endif

#if defined(Q_OS_MAC)
    QString imageSuffix(qgetenv("DYLD_IMAGE_SUFFIX"));

    // Bundle plugin directory
    CFBundleRef mainBundle = CFBundleGetMainBundle();
    if (mainBundle != 0) {
        CFURLRef baseUrl = CFBundleCopyBundleURL(mainBundle);
        CFURLRef pluginUrlPart = CFBundleCopyBuiltInPlugInsURL(mainBundle);
        CFStringRef pluginPathPart = CFURLCopyFileSystemPath(pluginUrlPart, kCFURLPOSIXPathStyle);
        CFURLRef pluginUrl = CFURLCreateCopyAppendingPathComponent(0, baseUrl, pluginPathPart, true);
        CFStringRef pluginPath = CFURLCopyFileSystemPath(pluginUrl, kCFURLPOSIXPathStyle);

        CFIndex length = CFStringGetLength(pluginPath);
        UniChar buffer[length];
        CFStringGetCharacters(pluginPath, CFRangeMake(0, length), buffer);

        paths << QString(reinterpret_cast<const QChar *>(buffer), length);

        CFRelease(pluginPath);
        CFRelease(pluginUrl);
        CFRelease(pluginPathPart);
        CFRelease(pluginUrlPart);
        CFRelease(baseUrl);
    }
#endif

    // Qt paths
    paths << QCoreApplication::libraryPaths();

    foreach (const QString &path, paths) {
        QDir typeDir(path + m_location);
#if defined(Q_OS_SYMBIAN)
        if (dirChecker.checkDir(typeDir))
#endif
        {
            foreach (const QString &file, typeDir.entryList(QDir::Files)) {
#if defined(Q_OS_MAC)
                if (!imageSuffix.isEmpty()) {   // Only add appropriate images
                    if (file.lastIndexOf(imageSuffix, -6) == -1)
                        continue;
                } else {  // Ignore any images with common suffixes
                    if (file.endsWith(QLatin1String("_debug.dylib")) ||
                        file.endsWith(QLatin1String("_profile.dylib")))
                        continue;
                }
#elif defined(Q_OS_UNIX)
                // Ignore separate debug files
                if (file.endsWith(QLatin1String(".debug")))
                    continue;
#elif defined(Q_OS_WIN)
                // Ignore non-dlls
                if (!file.endsWith(QLatin1String(".dll"), Qt::CaseInsensitive))
                    continue;
#endif
                plugins << typeDir.absoluteFilePath(file);
            }
        }
    }

    return  plugins;
}

void QMediaPluginLoader::load()
{
    if (!m_instances.isEmpty())
        return;

#if !defined QT_NO_DEBUG
    const bool showDebug = qgetenv("QT_DEBUG_PLUGINS").toInt() > 0;
#endif

    if (staticMediaPlugins() && staticMediaPlugins()->contains(m_location)) {
        foreach(QObject *o, staticMediaPlugins()->value(m_location)) {
            if (o != 0 && o->qt_metacast(m_iid) != 0) {
                QFactoryInterface* p = qobject_cast<QFactoryInterface*>(o);
                if (p != 0) {
                    foreach (QString const &key, p->keys())
                        m_instances[key].append(o);
                }
            }
        }
    } else {
        QSet<QString> loadedPlugins;

        foreach (const QString &plugin, availablePlugins()) {
            QString fileName = QFileInfo(plugin).fileName();
            //don't try to load plugin with the same name if it's already loaded
            if (loadedPlugins.contains(fileName)) {
#if !defined QT_NO_DEBUG
                if (showDebug)
                    qDebug() << "Skip loading plugin" << plugin;
#endif
                continue;
            }

            QPluginLoader   loader(plugin);

            QObject *o = loader.instance();
            if (o != 0 && o->qt_metacast(m_iid) != 0) {
                QFactoryInterface* p = qobject_cast<QFactoryInterface*>(o);
                if (p != 0) {
                    foreach (const QString &key, p->keys())
                        m_instances[key].append(o);

                    loadedPlugins.insert(fileName);
#if !defined QT_NO_DEBUG
                    if (showDebug)
                        qDebug() << "Loaded plugin" << plugin << "services:" << p->keys();
#endif
                }

                continue;
            } else {
#if !defined QT_NO_DEBUG
                if (showDebug)
                    qWarning() << "QMediaPluginLoader: Failed to load plugin: " << plugin << loader.errorString();
#endif
            }

            delete o;
        }
    }
}

QT_END_NAMESPACE

