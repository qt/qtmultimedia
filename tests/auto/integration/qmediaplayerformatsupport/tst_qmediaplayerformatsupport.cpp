// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <private/mediabackendutils_p.h>
#include <QtTest/qtest.h>
#include <QtTest/qsignalspy.h>
#include <QtCore/qdebug.h>
#include <QtCore/qdiriterator.h>
#include <QtMultimedia/qmediaplayer.h>
#include <QtMultimedia/QVideoSink>

using namespace Qt::StringLiterals;

QT_USE_NAMESPACE

struct Fixture
{
    Fixture() { player.setVideoOutput(&videoOutput); }

    QVideoSink videoOutput;
    QMediaPlayer player;
    QSignalSpy errorOccurred{ &player, &QMediaPlayer::errorOccurred };
    QSignalSpy playbackStateChanged{ &player, &QMediaPlayer::playbackStateChanged };

    bool startedPlaying() const
    {
        return playbackStateChanged.contains(QList<QVariant>{ QMediaPlayer::PlayingState });
    }
};

void addTestData(QLatin1StringView dir)
{
    QDirIterator it(dir);
    while (it.hasNext()) {
        QString v = it.next();
        QTest::addRow("%s", v.toLatin1().data()) << QUrl{ "qrc" + v };
    }
}

class tst_qmediaplayerformatsupport : public QObject
{
    Q_OBJECT

public slots:
    void initTestCase()
    {
        QSKIP_IF_NOT_FFMPEG("Test is only intended for FFmpeg backend");
    }

private slots:
    void play_succeeds_withSupportedContainer_data()
    {
        QTest::addColumn<QUrl>("url");
        addTestData(":testdata/containers/supported"_L1);
    }

    void play_succeeds_withSupportedContainer()
    {
        QFETCH(const QUrl, url);

        Fixture f;
        f.player.setSource(url);
        f.player.play();

        QTRY_VERIFY(f.startedPlaying());

        // Log to understand failures in CI
        for (const QList<QVariant> &err : f.errorOccurred)
            qCritical() << "Unexpected failure detected:" << err[0] << "," << err[1];

#ifdef Q_OS_ANDROID
        QSKIP("QTBUG-125613 Limited format support on Android 14");
#endif

        QVERIFY(f.errorOccurred.empty());
    }

    void play_succeeds_withSupportedPixelFormats_data()
    {
        QTest::addColumn<QUrl>("url");
        addTestData(":testdata/pixel_formats/supported"_L1);
    }

    void play_succeeds_withSupportedPixelFormats()
    {
        QFETCH(const QUrl, url);

        Fixture f;
        f.player.setSource(url);
        f.player.play();

        QTRY_VERIFY(f.startedPlaying());

        // Log to understand failures in CI
        for (const QList<QVariant> &err : f.errorOccurred)
            qCritical() << "Unexpected failure detected:" << err[0] << "," << err[1];

#ifdef Q_OS_ANDROID
        QSKIP("QTBUG-125613 Limited format support on Android 14");
#endif

        QVERIFY(f.errorOccurred.empty());
    }

    void play_fails_withUnsupportedContainer_data()
    {
        QTest::addColumn<QUrl>("url");
        addTestData(":testdata/containers/unsupported"_L1);
    }

    void play_fails_withUnsupportedContainer()
    {
        QFETCH(const QUrl, url);

        Fixture f;
        f.player.setSource(url);
        f.player.play();

        QTRY_COMPARE_NE(f.player.error(), QMediaPlayer::NoError);
    }
};

QTEST_MAIN(tst_qmediaplayerformatsupport)

#include "tst_qmediaplayerformatsupport.moc"
