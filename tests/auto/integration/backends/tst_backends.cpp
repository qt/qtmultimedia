// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>
#include <QDebug>
#include <QtCore/qsysinfo.h>
#include <private/qplatformmediaintegration_p.h>

QT_USE_NAMESPACE

class tst_backends : public QObject
{
    Q_OBJECT

public slots:
    void initTestCase()
    {
        // Log operating system name and currently supported backends
        qDebug() << QSysInfo::prettyProductName() << "supports backends"
                 << QPlatformMediaIntegration::availableBackends().join(", ");
    }

private slots:
    void availableBackends_returns_expectedBackends_data()
    {
        QTest::addColumn<QStringList>("expectedBackends");
        QStringList backends;

#if defined(Q_OS_WIN)
        backends << "windows";
        if (QSysInfo::currentCpuArchitecture() == "x86_64")
            backends << "ffmpeg";
#elif defined(Q_OS_ANDROID)
        backends << "android" << "ffmpeg";
#elif defined(Q_OS_DARWIN)
        backends << "darwin" << "ffmpeg";
#elif defined(Q_OS_WASM)
        backends << "wasm";
#elif defined(Q_OS_QNX)
        backends << "qnx";
#else
        backends << "ffmpeg";
        if constexpr (QT_CONFIG(gstreamer))
            backends << "gstreamer";
#endif

        QTest::addRow("backends") << backends;
    }

    void availableBackends_returns_expectedBackends()
    {
        QFETCH(QStringList, expectedBackends);
        QStringList actualBackends = QPlatformMediaIntegration::availableBackends();
        for (const auto &expectedBackend : expectedBackends) {
            QVERIFY(actualBackends.contains(expectedBackend));
        }
    }
};

QTEST_MAIN(tst_backends)

#include "tst_backends.moc"
