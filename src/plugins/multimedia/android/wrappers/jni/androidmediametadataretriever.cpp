// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "androidmediametadataretriever_p.h"

#include <QtCore/QUrl>
#include <qdebug.h>
#include <QtCore/qcoreapplication.h>

QT_BEGIN_NAMESPACE

AndroidMediaMetadataRetriever::AndroidMediaMetadataRetriever()
{
    m_metadataRetriever = QJniObject("android/media/MediaMetadataRetriever");
}

AndroidMediaMetadataRetriever::~AndroidMediaMetadataRetriever()
{
    release();
}

QString AndroidMediaMetadataRetriever::extractMetadata(MetadataKey key)
{
    QString value;

    QJniObject metadata = m_metadataRetriever.callObjectMethod("extractMetadata",
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

    QJniEnvironment env;
    if (url.isLocalFile()) { // also includes qrc files (copied to a temp file by QMediaPlayer)
        QJniObject string = QJniObject::fromString(url.path());
        QJniObject fileInputStream("java/io/FileInputStream",
                                   "(Ljava/lang/String;)V",
                                   string.object());

        if (!fileInputStream.isValid())
            return false;

        QJniObject fd = fileInputStream.callObjectMethod("getFD",
                                                         "()Ljava/io/FileDescriptor;");
        if (!fd.isValid()) {
            fileInputStream.callMethod<void>("close");
            return false;
        }

        auto methodId = env->GetMethodID(m_metadataRetriever.objectClass(), "setDataSource",
                                         "(Ljava/io/FileDescriptor;)V");
        env->CallVoidMethod(m_metadataRetriever.object(), methodId, fd.object());
        bool ok = !env.checkAndClearExceptions();
        fileInputStream.callMethod<void>("close");
        if (!ok)
            return false;
    } else if (url.scheme() == QLatin1String("assets")) {
        QJniObject string = QJniObject::fromString(url.path().mid(1)); // remove first '/'
        QJniObject activity(QNativeInterface::QAndroidApplication::context());
        QJniObject assetManager = activity.callObjectMethod("getAssets",
                                                            "()Landroid/content/res/AssetManager;");
        QJniObject assetFd = assetManager.callObjectMethod("openFd",
                                                           "(Ljava/lang/String;)Landroid/content/res/AssetFileDescriptor;",
                                                           string.object());
        if (!assetFd.isValid())
            return false;

        QJniObject fd = assetFd.callObjectMethod("getFileDescriptor",
                                                        "()Ljava/io/FileDescriptor;");
        if (!fd.isValid()) {
            assetFd.callMethod<void>("close");
            return false;
        }

        auto methodId = env->GetMethodID(m_metadataRetriever.objectClass(), "setDataSource",
                                         "(Ljava/io/FileDescriptor;JJ)V");
        env->CallVoidMethod(m_metadataRetriever.object(), methodId,
                            fd.object(),
                            assetFd.callMethod<jlong>("getStartOffset"),
                            assetFd.callMethod<jlong>("getLength"));
        bool ok = !env.checkAndClearExceptions();
        assetFd.callMethod<void>("close");

        if (!ok)
            return false;
    } else if (url.scheme() != QLatin1String("content")) {
        // On API levels >= 14, only setDataSource(String, Map<String, String>) accepts remote media
        QJniObject string = QJniObject::fromString(url.toString(QUrl::FullyEncoded));
        QJniObject hash("java/util/HashMap");

        auto methodId = env->GetMethodID(m_metadataRetriever.objectClass(), "setDataSource",
                                         "(Ljava/lang/String;Ljava/util/Map;)V");
        env->CallVoidMethod(m_metadataRetriever.object(), methodId,
                            string.object(), hash.object());
        if (env.checkAndClearExceptions())
            return false;
    } else {
        // While on API levels < 14, only setDataSource(Context, Uri) is available and works for
        // remote media...
        QJniObject string = QJniObject::fromString(url.toString(QUrl::FullyEncoded));
        QJniObject uri = m_metadataRetriever.callStaticObjectMethod(
                                                        "android/net/Uri",
                                                        "parse",
                                                        "(Ljava/lang/String;)Landroid/net/Uri;",
                                                        string.object());
        if (!uri.isValid())
            return false;

        auto methodId = env->GetMethodID(m_metadataRetriever.objectClass(), "setDataSource",
                                         "(Landroid/content/Context;Landroid/net/Uri;)V");
        env->CallVoidMethod(m_metadataRetriever.object(), methodId,
                            QNativeInterface::QAndroidApplication::context(), uri.object());
        if (env.checkAndClearExceptions())
            return false;
    }

    return true;
}

QT_END_NAMESPACE
