// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "mediabackendutils_p.h"

#include <QtCore/qbytearray.h>
#include <QtCore/qlist.h>

namespace BackendUtilsImpl {

void addMediaBackendToTestEnvironment(const QString &backend)
{
    const QByteArray backendName = backend.toLower().toUtf8();
    const QByteArray backendKey = backendName + "-mediabackend";
    QByteArray testEnvironment = qgetenv("QTEST_ENVIRONMENT");
    bool hasBackendKey = false;
    const QList<QByteArray> keys = testEnvironment.split(' ');
    for (const QByteArray &key : keys) {
        if (!key.endsWith("-mediabackend"))
            continue;
        if (key != backendKey) {
            qFatal(
                "QTEST_ENVIRONMENT contains '%s', but the media backend in use is '%s'",
                key.constData(),
                backendName.constData());
        }
        hasBackendKey = true;
    }
    if (!hasBackendKey) {
        if (!testEnvironment.isEmpty())
            testEnvironment += ' ';
        testEnvironment += backendKey;
        qputenv("QTEST_ENVIRONMENT", testEnvironment);
    }
}

} // namespace BackendUtilsImpl
