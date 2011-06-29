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

#include "tst_qmediaobject_xa.h"

QT_USE_NAMESPACE

#define WAIT_FOR_CONDITION(a,e)            \
    for (int _i = 0; _i < 500; _i += 1) {  \
        if ((a) == (e)) break;             \
        QTest::qWait(10);}


#define WAIT_LONG_FOR_CONDITION(a,e)        \
    for (int _i = 0; _i < 1800; _i += 1) {  \
        if ((a) == (e)) break;              \
        QTest::qWait(100);}

Q_DECLARE_METATYPE(QtMultimediaKit::MetaData)

void tst_QMetadata_xa::initTestCase_data()
{
    QTest::addColumn<QMediaContent>("mediaContent");
    QTest::addColumn<int>("count");
    QTest::addColumn<QtMultimediaKit::MetaData>("key");
    QTest::addColumn<qint64>("intValue");
    QTest::addColumn<QString>("strValue");

    QTest::newRow("testmp3.mp3 - No Metadata")
    << QMediaContent(QUrl("file:///C:/data/testfiles/test.mp3")) // mediaContent
    << 0 //count
    << QtMultimediaKit::Title //key - irrelavant when count 0
    << qint64(-1) //intValue
    << QString(); //strValue

    QTest::newRow("JapJap.mp3 - Title")
    << QMediaContent(QUrl("file:///C:/data/testfiles/JapJap.mp3")) // mediaContent
    << -1 //count - ignore
    << QtMultimediaKit::Title //key
    << qint64(-1) //intValue
    << QString("JapJap");//strValue

    QTest::newRow("JapJap.mp3 - Artist")
    << QMediaContent(QUrl("file:///C:/data/testfiles/JapJap.mp3")) // mediaContent
    << -1 //count - ignore
    << QtMultimediaKit::AlbumArtist //key
    << qint64(-1) //intValue
    << QString("Screaming trees");//strValue

    QTest::newRow("JapJap.mp3 - Album")
    << QMediaContent(QUrl("file:///C:/data/testfiles/JapJap.mp3")) // mediaContent
    << -1 //count - ignore
    << QtMultimediaKit::AlbumTitle //key
    << qint64(-1) //intValue
    << QString("Sweet oblivion"); //strValue

    QTest::newRow("JapJap.mp3 - CoverArt")
    << QMediaContent(QUrl("file:///C:/data/testfiles/JapJap.mp3")) // mediaContent
    << -1 //count - ignore
    << QtMultimediaKit::CoverArtImage //key
    << qint64(28521) //intValue
    << QString("C:/data/testfiles/JapJapCoverArt.jpeg"); //strValue
}

void tst_QMetadata_xa::initTestCase()
{
    m_player = new QMediaPlayer();

    // Symbian back end needs coecontrol for creation.
    m_widget = new QVideoWidget();
    //m_widget->setMediaObject(m_player);
    m_widget->show();
    runonce = false;
}

void tst_QMetadata_xa::cleanupTestCase()
{
    delete m_player;
    delete m_widget;
}

void tst_QMetadata_xa::init()
{
    qRegisterMetaType<QMediaContent>("QMediaContent");
    qRegisterMetaType<QtMultimediaKit::MetaData>("QtMultimediaKit::MetaData");
}

void tst_QMetadata_xa::cleanup()
{
}

void tst_QMetadata_xa::testMetadata()
{
    QFETCH_GLOBAL(QMediaContent, mediaContent);
    QFETCH_GLOBAL(int, count);
    QFETCH_GLOBAL(QtMultimediaKit::MetaData, key);
    QFETCH_GLOBAL(QString, strValue);
    QFETCH_GLOBAL(qint64, intValue);

    QSignalSpy spy(m_player, SIGNAL(metaDataAvailableChanged(bool)));
    m_player->setMedia(mediaContent);

    WAIT_FOR_CONDITION(spy.count(), 1);
    //get metadata count
    QList<QtMultimediaKit::MetaData> mdList = m_player->availableMetaData ();
    QStringList amdList = m_player->availableExtendedMetaData();

    int numMetadataItems = mdList.size() + amdList.size();
    if (count>=0) //-1 indicate ignore count
        QVERIFY(count==numMetadataItems);

    if (numMetadataItems>0 && !strValue.isEmpty()) {
        QVariant val = m_player->metaData(key);

        if(key == QtMultimediaKit::CoverArtImage)
            val.value<QImage>().save(strValue);
        else
            QVERIFY(strValue == val.toString());
    }
}
