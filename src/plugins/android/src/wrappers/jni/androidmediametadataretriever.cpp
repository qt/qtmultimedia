/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "androidmediametadataretriever.h"

#include <QtCore/private/qjnihelpers_p.h>
#include <QtCore/private/qjni_p.h>

QT_BEGIN_NAMESPACE

AndroidMediaMetadataRetriever::AndroidMediaMetadataRetriever()
{
    m_metadataRetriever = QJNIObjectPrivate("android/media/MediaMetadataRetriever");
}

AndroidMediaMetadataRetriever::~AndroidMediaMetadataRetriever()
{
}

QString AndroidMediaMetadataRetriever::extractMetadata(MetadataKey key)
{
    QString value;

    QJNIObjectPrivate metadata = m_metadataRetriever.callObjectMethod("extractMetadata",
                                                                      "(I)Ljava/lang/String;",
                                                                      jint(key));
    if (metadata.isValid())
        value = metadata.toString();

    return value;
}

void AndroidMediaMetadataRetriever::release()
{
    if (!m_metadataRetriever.isValid())
        return;

    m_metadataRetriever.callMethod<void>("release");
}

bool AndroidMediaMetadataRetriever::setDataSource(const QUrl &url)
{
    if (!m_metadataRetriever.isValid())
        return false;

    QJNIEnvironmentPrivate env;

    bool loaded = false;

    QJNIObjectPrivate string = QJNIObjectPrivate::fromString(url.toString());

    QJNIObjectPrivate uri = m_metadataRetriever.callStaticObjectMethod("android/net/Uri",
                                                                       "parse",
                                                                       "(Ljava/lang/String;)Landroid/net/Uri;",
                                                                       string.object());
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
    } else {
        m_metadataRetriever.callMethod<void>("setDataSource",
                                             "(Landroid/content/Context;Landroid/net/Uri;)V",
                                             QtAndroidPrivate::activity(),
                                             uri.object());
        if (env->ExceptionCheck())
            env->ExceptionClear();
        else
            loaded = true;
    }

    return loaded;
}

bool AndroidMediaMetadataRetriever::setDataSource(const QString &path)
{
    if (!m_metadataRetriever.isValid())
        return false;

    QJNIEnvironmentPrivate env;

    bool loaded = false;

    m_metadataRetriever.callMethod<void>("setDataSource",
                                         "(Ljava/lang/String;)V",
                                         QJNIObjectPrivate::fromString(path).object());
    if (env->ExceptionCheck())
        env->ExceptionClear();
    else
        loaded = true;

    return loaded;
}

QT_END_NAMESPACE
