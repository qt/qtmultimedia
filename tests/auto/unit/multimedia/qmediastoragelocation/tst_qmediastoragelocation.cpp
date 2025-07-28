// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>
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

    void generateFileName_generatesFileNameWithProperIndexAndExtension_whenInvokedForDirectory_data()
    {
        QTest::addColumn<QStandardPaths::StandardLocation>("locationType");
        QTest::addColumn<QString>("extension");
        QTest::addColumn<QString>("expectedFilePattern");

        QTest::addRow("Music location, with extension")
            << QStandardPaths::MusicLocation << QStringLiteral("myext") << QStringLiteral("record_000%1.myext");
        QTest::addRow("Music location, without extension")
            << QStandardPaths::MusicLocation << QString() << QStringLiteral("record_000%1");

        QTest::addRow("Movies location, with extension")
            << QStandardPaths::MoviesLocation << QStringLiteral("myext") << QStringLiteral("video_000%1.myext");
        QTest::addRow("Movies location, without extension")
            << QStandardPaths::MoviesLocation << QString() << QStringLiteral("video_000%1");

        QTest::addRow("Pictures location, with extension")
            << QStandardPaths::PicturesLocation << QStringLiteral("myext") << QStringLiteral("image_000%1.myext");
        QTest::addRow("Pictures location, without extension")
            << QStandardPaths::PicturesLocation << QString() << QStringLiteral("image_000%1");

        QTest::addRow("Any location, with extension")
            << QStandardPaths::TempLocation << QStringLiteral("myext") << QStringLiteral("clip_000%1.myext");
        QTest::addRow("Any location, without extension")
            << QStandardPaths::TempLocation << QString() << QStringLiteral("clip_000%1");
    }

    void generateFileName_generatesFileNameWithProperIndexAndExtension_whenInvokedForDirectory()
    {
        QFETCH(const QStandardPaths::StandardLocation, locationType);
        QFETCH(const QString, extension);
        QFETCH(const QString, expectedFilePattern);

        QTemporaryDir tempDir;

        auto generateFileName = [&]() {
            return QMediaStorageLocation::generateFileName(tempDir.path(), locationType, extension);
        };

        auto createFile = [&](int index) {
            QTEST_ASSERT(index < 10);
            QFile file(tempDir.filePath(expectedFilePattern.arg(index)));
            QTEST_ASSERT(file.open(QFile::WriteOnly));
        };

        const QString fileName_1 = generateFileName();

        QCOMPARE(fileName_1, tempDir.filePath(expectedFilePattern.arg(1)));
        QCOMPARE(fileName_1, generateFileName()); // generates the same name 2nd time

        createFile(1);
        const QString fileName_2 = generateFileName();
        QCOMPARE(fileName_2, tempDir.filePath(expectedFilePattern.arg(2)));

        createFile(8);
        const QString fileName_9 = generateFileName();
        QCOMPARE(fileName_9, tempDir.filePath(expectedFilePattern.arg(9)));
    }
};

QTEST_GUILESS_MAIN(tst_qmediastoragelocation)
#include "tst_qmediastoragelocation.moc"
