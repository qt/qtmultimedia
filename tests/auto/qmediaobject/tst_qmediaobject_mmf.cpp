/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
#include <tst_qmediaobject_mmf.h>

QT_USE_NAMESPACE

void tst_QMediaObject_mmf::initTestCase_data()
{
    QTest::addColumn<bool>("valid");
    QTest::addColumn<QMediaContent>("mediaContent");
    QTest::addColumn<bool>("metaDataAvailable");

    QTest::newRow("TestDataNull")
    << false // valid
    << QMediaContent() // mediaContent
    << false; // metaDataAvailable

    QTest::newRow("test_amr.amr")
    << true // valid
    << QMediaContent(QUrl("file:///C:/data/testfiles/test_amr.amr")) // mediaContent
    << false; // metaDataAvailable

    QTest::newRow("test_flash_video.flv")
    << true // valid
    << QMediaContent(QUrl("file:///C:/data/testfiles/test_flash_video.flv")) // mediaContent
    << false; // metaDataAvailable

    QTest::newRow("test_invalid_extension_mp4.xyz")
    << true // valid
    << QMediaContent(QUrl("file:///C:/data/testfiles/test_invalid_extension_mp4.xyz")) // mediaContent
    << false; // metaDataAvailable

    QTest::newRow("test_invalid_extension_wav.xyz")
    << true // valid
    << QMediaContent(QUrl("file:///C:/data/testfiles/test_invalid_extension_wav.xyz")) // mediaContent
    << false; // metaDataAvailable

    QTest::newRow("test_mp3.mp3")
    << true // valid
    << QMediaContent(QUrl("file:///C:/data/testfiles/test_mp3.mp3")) // mediaContent
#if !defined(__WINS__) || !defined(__WINSCW__)
    << true; // metaDataAvailable
#else
    << false; // metaDataAvailable
#endif // !defined(__WINS__) || defined(__WINSCW__)

    QTest::newRow("test_mp3_no_metadata.mp3")
    << true // valid
    << QMediaContent(QUrl("file:///C:/data/testfiles/test_mp3_no_metadata.mp3")) // mediaContent
    << false; // metaDataAvailable

    QTest::newRow("test_mp4.mp4")
    << true // valid
    << QMediaContent(QUrl("file:///C:/data/testfiles/test_mp4.mp4")) // mediaContent
#if defined(__WINS__) || defined(__WINSCW__)
    << true; // metaDataAvailable
#else
    << false; // metaDataAvailable
#endif // !defined(__WINS__) || defined(__WINSCW__)

    QTest::newRow("test_wav.wav")
    << true // valid
    << QMediaContent(QUrl("file:///C:/data/testfiles/test_wav.wav")) // mediaContent
    << false; // metaDataAvailable

    QTest::newRow("test_wmv9.wmv")
    << true // valid
    << QMediaContent(QUrl("file:///C:/data/testfiles/test_wmv9.wmv")) // mediaContent
    << false; // metaDataAvailable

    QTest::newRow("test youtube stream")
    << true // valid
       << QMediaContent(QUrl("rtsp://v3.cache4.c.youtube.com/CkgLENy73wIaPwlU2rm7yu8PFhMYESARFEIJbXYtZ29vZ2xlSARSB3JlbGF0ZWRaDkNsaWNrVGh1bWJuYWlsYPi6_IXT2rvpSgw=/0/0/0/video.3gp")) // mediaContent
    << false; // metaDataAvailable
}

void tst_QMediaObject_mmf::initTestCase()
{
}

void tst_QMediaObject_mmf::cleanupTestCase()
{
}

void tst_QMediaObject_mmf::init()
{
    qRegisterMetaType<QMediaContent>("QMediaContent");
}

void tst_QMediaObject_mmf::cleanup()
{
}

void tst_QMediaObject_mmf::isMetaDataAvailable()
{
    QFETCH_GLOBAL(QMediaContent, mediaContent);
    QFETCH_GLOBAL(bool, metaDataAvailable);
    QMediaPlayer player;

    player.setMedia(mediaContent);
    QTest::qWait(700);
    QVERIFY(player.isMetaDataAvailable() == metaDataAvailable);
}

void tst_QMediaObject_mmf::metaData()
{
    QFETCH_GLOBAL(QMediaContent, mediaContent);
    QFETCH_GLOBAL(bool, metaDataAvailable);
    QMediaPlayer player;

    player.setMedia(mediaContent);
    QTest::qWait(700);
    const QString artist(QLatin1String("Artist"));
    const QString title(QLatin1String("Title"));

    if (player.isMetaDataAvailable()) {
        QCOMPARE(player.metaData(QtMultimediaKit::AlbumArtist).toString(), artist);
        QCOMPARE(player.metaData(QtMultimediaKit::Title).toString(), title);
    }
}

void tst_QMediaObject_mmf::availableMetaData()
{
    QFETCH_GLOBAL(QMediaContent, mediaContent);
    QFETCH_GLOBAL(bool, metaDataAvailable);
    QMediaPlayer player;

    player.setMedia(mediaContent);
    QTest::qWait(700);

    if (player.isMetaDataAvailable()) {
        QList<QtMultimediaKit::MetaData> metaDataKeys = player.availableMetaData();
        QVERIFY(metaDataKeys.count() > 0);
        QVERIFY(metaDataKeys.contains(QtMultimediaKit::AlbumArtist));
        QVERIFY(metaDataKeys.contains(QtMultimediaKit::Title));
    }
}

void tst_QMediaObject_mmf::extendedMetaData()
{
    QFETCH_GLOBAL(QMediaContent, mediaContent);
    QMediaPlayer player;

    player.setMedia(mediaContent);
    QTest::qWait(700);
    const QString artist(QLatin1String("Artist"));
    const QString title(QLatin1String("Title"));

    if (player.isMetaDataAvailable()) {
        QCOMPARE(player.extendedMetaData(metaDataKeyAsString(QtMultimediaKit::AlbumArtist)).toString(), artist);
        QCOMPARE(player.extendedMetaData(metaDataKeyAsString(QtMultimediaKit::Title)).toString(), title);
    }
}

void tst_QMediaObject_mmf::availableExtendedMetaData()
{
    QFETCH_GLOBAL(QMediaContent, mediaContent);
    QMediaPlayer player;

    player.setMedia(mediaContent);
    QTest::qWait(700);
    const QString artist(QLatin1String("Artist"));
    const QString title(QLatin1String("Title"));

    if (player.isMetaDataAvailable()) {
        QStringList metaDataKeys = player.availableExtendedMetaData();
        QVERIFY(metaDataKeys.count() > 0);
/*        qWarning() << "metaDataKeys.count: " << metaDataKeys.count();
        int count = metaDataKeys.count();
        count = count-1;
        int i = 0;
        while(count >= i)
            {
            qWarning() << "metaDataKeys "<<i<<"." << metaDataKeys.at(i);
            i++;
            }*/
        QVERIFY(metaDataKeys.contains(metaDataKeyAsString(QtMultimediaKit::AlbumArtist)));
        QVERIFY(metaDataKeys.contains(metaDataKeyAsString(QtMultimediaKit::Title)));
    }
}

QString tst_QMediaObject_mmf::metaDataKeyAsString(QtMultimediaKit::MetaData key) const
{
    switch(key) {
        case QtMultimediaKit::Title: return "title";
        case QtMultimediaKit::AlbumArtist: return "artist";
        case QtMultimediaKit::Comment: return "comment";
        case QtMultimediaKit::Genre: return "genre";
        case QtMultimediaKit::Year: return "year";
        case QtMultimediaKit::Copyright: return "copyright";
        case QtMultimediaKit::AlbumTitle: return "album";
        case QtMultimediaKit::Composer: return "composer";
        case QtMultimediaKit::TrackNumber: return "albumtrack";
        case QtMultimediaKit::AudioBitRate: return "audiobitrate";
        case QtMultimediaKit::VideoBitRate: return "videobitrate";
        case QtMultimediaKit::Duration: return "duration";
        case QtMultimediaKit::MediaType: return "contenttype";
        case QtMultimediaKit::SubTitle:
        case QtMultimediaKit::Description:
        case QtMultimediaKit::Category:
        case QtMultimediaKit::Date:
        case QtMultimediaKit::UserRating:
        case QtMultimediaKit::Keywords:
        case QtMultimediaKit::Language:
        case QtMultimediaKit::Publisher:
        case QtMultimediaKit::ParentalRating:
        case QtMultimediaKit::RatingOrganisation:
        case QtMultimediaKit::Size:
        case QtMultimediaKit::AudioCodec:
        case QtMultimediaKit::AverageLevel:
        case QtMultimediaKit::ChannelCount:
        case QtMultimediaKit::PeakValue:
        case QtMultimediaKit::SampleRate:
        case QtMultimediaKit::Author:
        case QtMultimediaKit::ContributingArtist:
        case QtMultimediaKit::Conductor:
        case QtMultimediaKit::Lyrics:
        case QtMultimediaKit::Mood:
        case QtMultimediaKit::TrackCount:
        case QtMultimediaKit::CoverArtUrlSmall:
        case QtMultimediaKit::CoverArtUrlLarge:
        case QtMultimediaKit::Resolution:
        case QtMultimediaKit::PixelAspectRatio:
        case QtMultimediaKit::VideoFrameRate:
        case QtMultimediaKit::VideoCodec:
        case QtMultimediaKit::PosterUrl:
        case QtMultimediaKit::ChapterNumber:
        case QtMultimediaKit::Director:
        case QtMultimediaKit::LeadPerformer:
        case QtMultimediaKit::Writer:
        case QtMultimediaKit::CameraManufacturer:
        case QtMultimediaKit::CameraModel:
        case QtMultimediaKit::Event:
        case QtMultimediaKit::Subject:
        default:
            break;
    }

    return QString();
}
