// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QtTest>
#include <private/qmediastoragelocation_p.h>

QT_USE_NAMESPACE

class tst_qmediastoragelocation : public QObject
{
    Q_OBJECT

private slots:
    void generateFileName_addsExtension_onlyWhenExtensionIsMissingOrWrong_data()
    {
        QTest::addColumn<QString>("filename");
        QTest::addColumn<QString>("extension");
        QTest::addColumn<QString>("expected");

        // clang-format off

        QTest::addRow("Extension is added when input has no extension")
            << "filename" << "ext" << "filename.ext";

        QTest::addRow("Extension is not added when input has correct extension")
            << "filename.ext" << "ext" << "filename.ext";

        QTest::addRow("Extension is not added when input has wrong extension")
            << "filename.jpg" << "ext" << "filename.jpg";

        QTest::addRow("Extension is added when input is empty")
            << "" << "ext" << ".ext";

        QTest::addRow("Extension is not added when extension is empty")
            << "filename" << "" << "filename";

        QTest::addRow("Extension is added without extra dot when filename ends with dot")
            << "file." << "ext" << "file.ext";

        // clang-format on
    }

    void generateFileName_addsExtension_onlyWhenExtensionIsMissingOrWrong()
    {
        QFETCH(const QString, filename);
        QFETCH(const QString, extension);
        QFETCH(const QString, expected);

        const QString path = QMediaStorageLocation::generateFileName(
                filename, QStandardPaths::TempLocation, extension);

        const bool pass = path.endsWith(expected);
        if (!pass)
            qWarning() << "Expected path to end with" << expected << "but got" << path;

        QVERIFY(pass);
    }
};

QTEST_GUILESS_MAIN(tst_qmediastoragelocation)
#include "tst_qmediastoragelocation.moc"
