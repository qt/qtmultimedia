// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qandroidcacertificates_p.h"

#include <QtFFmpegMediaPluginImpl/private/qffmpegcertificateutils_p.h>

#include <QtCore/qdir.h>
#include <QtCore/qfile.h>
#include <QtCore/qjniarray.h>
#include <QtCore/qjniobject.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qsavefile.h>
#include <QtCore/qstandardpaths.h>

#include <jni.h>

#include <atomic>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

Q_DECLARE_JNI_CLASS(QtSslCertificates, "org/qtproject/qt/android/multimedia/qffmpeg/QtSslCertificates")

namespace QFFmpeg {

Q_STATIC_LOGGING_CATEGORY(qLCAndroidCaCertificates, "qt.multimedia.ffmpeg.android.caCertificates");

static QList<QByteArray> fetchAndroidSystemCertificates()
{
    QList<QByteArray> derCertificates;

    auto certificates =
            QtJniTypes::QtSslCertificates::callStaticMethod<QJniArray<QJniArray<jbyte>>>(
                    "getCertificates");
    if (!certificates.isValid())
        return derCertificates;

    derCertificates.reserve(certificates.size());
    for (const auto &jCertificate : certificates)
        derCertificates.append(jCertificate->toContainer());

    return derCertificates;
}

QString androidFFmpegCaBundlePath()
{
    static const QByteArray pemBundle = certificatesToPemBundle(fetchAndroidSystemCertificates());

    if (pemBundle.isEmpty()) {
        qCWarning(qLCAndroidCaCertificates) << "Could not obtain any system CA certificates";
        return {};
    }

    const QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    QString path = cacheDir + "/ffmpeg_ca_bundle.pem"_L1;

    // A bundle left behind by a previous run may predate a system trust store update, so
    // rewrite it once per process. After that, only rewrite it if it got evicted.
    // Concurrent callers may both rewrite it, which is harmless: QSaveFile replaces the file
    // atomically, so writers and readers never see a partially written file.
    static std::atomic_bool writtenInThisProcess = false;
    if (writtenInThisProcess.load(std::memory_order_relaxed) && QFile::exists(path))
        return path;

    QDir().mkpath(cacheDir);

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(pemBundle) != pemBundle.size()
        || !file.commit()) {
        qCWarning(qLCAndroidCaCertificates) << "Failed to write CA bundle to" << path;
        return {};
    }

    writtenInThisProcess.store(true, std::memory_order_relaxed);
    return path;
}

} // namespace QFFmpeg

QT_END_NAMESPACE
